# 04 按键与定时扫描

[模块目录](README.md) · [上一章](03-assets.md) · [下一章](05-rtc.md)

## 设计思想

三个层次分开：`KEY_GetValue`读电平，`KEY_Scan`按周期生成事件，`Key_Event`把事件交给当前页面。采用松手触发，按住不重复产生“确认”。中断只采样，不在中断里画屏幕或切页面。

## 变量与逻辑

| 变量 | 初值/范围 | 功能 |
|---|---|---|
| `cnt` | 0，累加到20归零 | 把1ms中断分成20ms采样 |
| `preKey` | 0～3 | 上一次采样的键号 |
| `curKey` | 0～3 | 本次采样的键号，0表示松开 |
| `key_value` | `static volatile`，初值0 | 待页面消费的单槽事件 |
| `primask` | 保存进入函数前中断屏蔽状态 | 退出临界区时恢复原状态 |

```text
中断每1ms：cnt++ → 到20？ → preKey=curKey → 读新curKey
若preKey!=0且curKey==0：key_value=preKey
页面读取：短暂关中断 → 复制key_value并清0 → 恢复中断状态
```

这是20ms间隔采样，不是要求连续多次相同的严格消抖。单槽也不是队列，页面长时间不消费时新事件可能覆盖旧事件。

## A 移植：借用规则，重新接硬件

1. 原 `Key_GetState/Key_Tick/Key_GetNum`分别对应当前三个接口，先理解含义再改名。
2. GPIO读取换成HAL；PB1为键1，PA6键2，PA4键3，按下均为低电平。
3. TIM2初始化和IRQ由CubeMX生成，不复制原72MHz工程的参数。
4. 在Keil添加 `key.c`，头文件包含 `main.h`；完整对照 [key.h](reference/bsp/key.h)、[key.c](reference/bsp/key.c)。

## B 自己编码：按输入→采样→消费的顺序

### 第1步：只写电平读取

```c
uint8_t KEY_GetValue(void)
{
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) return 1;
    else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET) return 2;
    else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET) return 3;
    return 0;
}
```

`else if`表示同时按键时按1、2、3的顺序优先判断，不支持组合键。先用调试器看返回值，再接采样。

### 第2步：定义状态并写周期采样

```c
static volatile uint8_t key_value = 0;
static uint8_t preKey = 0, curKey = 0, cnt = 0;

void KEY_Scan(void)
{
    if (++cnt >= 20) {
        cnt = 0;
        preKey = curKey;
        curKey = KEY_GetValue();
        if (preKey != 0 && curKey == 0) key_value = preKey;
    }
}
```

先在纸上代入采样序列 `0,1,1,0`：最后一次才生成事件1；连续松开不生成事件。

### 第3步：写读取并清空

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

`volatile`保证访问共享值，临界区保护“读取+清空”这个整体，避免中断在两条语句间写入的新事件被清掉。恢复原屏蔽状态，不能无条件开启中断。

### 第4步：接通中断链

在 `main.c`的 `USER CODE BEGIN 4`放唯一HAL回调：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) KEY_Scan();
}
```

检查生成的 [TIM2_IRQHandler](reference/Core/Src/stm32f1xx_it.c)调用 `HAL_TIM_IRQHandler(&htim2)`；正式启动由 `App_Init`执行。初次单独测试可在外设初始化后清更新标志，再启动一次中断，整合后删除重复启动。

## 验收与边界

暂时让主循环将非零 `Key_Event`显示到OLED：一次按下松开只出现一个事件，按住不重复。页面每轮只消费一次并存入局部 `key`，不要在多个 `if`里反复调用消费函数。后续若增加长按或组合键，需要新状态机，而非在页面里猜测。
