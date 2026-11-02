# 05 按键与定时扫描

[上一章](04-fonts.md) · [返回目录](../README.md) · [下一章：首页](06-home.md)

## 本章目标

一次按下、松开，只产生一个键值。沿用作者的方案：1 ms服务、20 ms间隔采样、松手触发、页面领取事件。

## 1. 分清三个函数

| 函数 | 责任 | 调用位置 |
|---|---|---|
| `KEY_GetValue()` | 读取现在按着哪个键 | 扫描函数 |
| `KEY_Scan()` | 比较前后采样，生成松手事件 | TIM2回调 |
| `Key_Event()` | 取出并清空待处理事件 | 当前页面循环 |

这里使用间隔采样降低机械抖动的影响。它不保证输入已经连续稳定20 ms，也不能可靠识别短于采样间隔的动作。先理解并复现原作者的方法，再扩展更严格的消抖或长按逻辑。

## 2. 头文件

新增 `bsp/key.h`，并确保文件末尾有换行：

```c
#ifndef KEY_H
#define KEY_H

#include "main.h"

uint8_t KEY_GetValue(void);
void KEY_Scan(void);
uint8_t Key_Event(void);

#endif
```

## 3. 原始读取

`bsp/key.c`：

```c
#include "key.h"

static volatile uint8_t key_value = 0;
static uint8_t preKey = 0;
static uint8_t curKey = 0;
static uint8_t cnt = 0;

uint8_t KEY_GetValue(void)
{
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        return 1;
    }
    else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET)
    {
        return 2;
    }
    else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
    {
        return 3;
    }

    return 0;
}
```

上拉输入松开为高、按下为低。`0` 表示没有按键；其他值表示当前按着的按键。

同时按多个键时，这个函数优先返回前面的键号。当前阶段按单键交互设计。

## 4. 定期识别松手

继续在 `key.c` 中添加：

```c
void KEY_Scan(void)
{
    cnt++;

    if (cnt >= 20)
    {
        cnt = 0;

        preKey = curKey;
        curKey = KEY_GetValue();

        if (preKey != 0 && curKey == 0)
        {
            key_value = preKey;
        }
    }
}
```

两次采样的含义：

| 上次 | 本次 | 含义 |
|---|---|---|
| 0 | 1 | 键1按下 |
| 1 | 1 | 键1按住 |
| 1 | 0 | 键1松开，写入事件1 |
| 0 | 0 | 没有动作 |

`preKey/curKey` 必须在20次分频条件内更新。若每1 ms更新状态、却只在第20次检查边沿，其他时刻的松手边沿会被覆盖。

## 5. 页面取走事件

继续添加：

```c
uint8_t Key_Event(void)
{
    uint32_t primask = __get_PRIMASK();
    uint8_t key;

    __disable_irq();
    key = key_value;
    key_value = 0;
    __set_PRIMASK(primask);

    return key;
}
```

`key_value` 是中断和主循环共享的单槽信箱：

- `volatile` 使共享值的访问按要求发生；它本身不把两条语句变成原子操作。
- 短暂保护“读取、清零”，避免中断恰好在两步之间写入新事件。
- 保存并恢复原中断状态，避免无条件开启原本已经关闭的中断。

信箱只能保留一个待处理键值。页面应及时领取；后续若需要保存连续多个事件，再扩展为队列。

## 6. 连接TIM2

在 `main.c` 用户包含区加入 `#include "key.h"`。

CubeMX 的 `TIM2_IRQHandler()` 保留对 `HAL_TIM_IRQHandler(&htim2)` 的调用。在 `USER CODE BEGIN 4` 中定义回调：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        KEY_Scan();
    }
}
```

在外设和 OLED 初始化之后启动：

```c
__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
{
    Error_Handler();
}
```

初始化函数只配置定时器，`HAL_TIM_Base_Start_IT()` 才启动带中断的运行。不要在回调中刷新整屏、等待按键松开或调用 `HAL_Delay()`。

## 7. 独立验证

在主循环中临时测试：

```c
uint8_t key = Key_Event();

if (key != 0)
{
    OLED_ShowNum(112, 0, key, 1, OLED_8X16);
    OLED_Update();
}
```

验证三键分别显示1、2、3，按住不连续触发，松手触发一次。完成后用下一章的页面循环替换这段测试代码。

同一层循环每次只调用一次 `Key_Event()`，将结果保存在局部变量中使用；连续调用会把事件提前消费掉。
