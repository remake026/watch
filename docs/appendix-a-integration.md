# 附录 A：当前阶段整合代码

[返回目录](../README.md) · [年份编辑](09-year.md)

本附录汇总第05～09章完成后的代码关系。它是当前阶段的对照快照，代码块按下列文件和区域放置，避免重复定义同名函数或变量。

保留 CubeMX 生成的 `SystemClock_Config()`、`Error_Handler()` 和各外设文件。下面的 main 只展示调用结构，不能用它删除原文件中其他生成内容。

当前仅年份可编辑；其他字段可选，确认编辑尚未接入。时间仍来自 RAM 数组。

## key.h

```c
#ifndef KEY_H
#define KEY_H

#include "main.h"

uint8_t KEY_GetValue(void);
void KEY_Scan(void);
uint8_t Key_Event(void);

#endif
```

## key.c

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

## main.c 的包含区

新增库头文件放在用户包含区；CubeMX 已生成的包含项保持单份。

```c
#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "OLED.h"
#include "key.h"
```

## main.c 的宏和全局变量

宏放入 `USER CODE BEGIN PD`，变量放入 `USER CODE BEGIN PV`。

```c
#define FIX_YEAR 2000

uint8_t my_time[] = {26, 11, 2, 21, 13, 0};
uint8_t cliFlag = 1;
uint8_t setFlag = 1;
uint8_t timeFlag = 1;
```

## main.c 的用户函数

这些函数按下列顺序放入 `USER CODE BEGIN 0`，位于 main 前面。顺序保证每个函数调用之前已有定义。

```c
static void Show_Ui(void)
{
    OLED_Clear();

    OLED_Printf(0, 0, OLED_8X16, "%04d-%02d-%02d",
                FIX_YEAR + my_time[0], my_time[1], my_time[2]);

    OLED_Printf(16, 16, OLED_12X24, "%02d:%02d:%02d",
                my_time[3], my_time[4], my_time[5]);

    OLED_ShowString(0, 40, "菜单", OLED_12X24);
    OLED_ShowString(80, 40, "设置", OLED_12X24);

    if (cliFlag == 1)
    {
        OLED_ReverseArea(0, 40, 48, 24);
    }
    else
    {
        OLED_ReverseArea(80, 40, 48, 24);
    }

    OLED_Update();
}

static uint8_t First_Page_Clock(void)
{
    while (1)
    {
        uint8_t key = Key_Event();

        if (key == 1)
        {
            if (cliFlag > 1) cliFlag--;
            else cliFlag = 2;
        }
        else if (key == 2)
        {
            if (cliFlag < 2) cliFlag++;
            else cliFlag = 1;
        }
        else if (key == 3)
        {
            return cliFlag;
        }

        Show_Ui();
    }
}

static void Show_SettingPage_UI(void)
{
    OLED_Clear();

    OLED_ShowString(0, 0, "< Back", OLED_8X16);
    OLED_ShowString(0, 24, "Date / Time", OLED_8X16);

    if (setFlag == 1)
    {
        OLED_ReverseArea(0, 0, 48, 16);
    }
    else
    {
        OLED_ReverseArea(0, 24, 88, 16);
    }

    OLED_Update();
}

static void Show_DateFields(void)
{
    OLED_ShowString(0, 0, "< Back", OLED_8X16);
    OLED_Printf(0, 16, OLED_8X16,
                "Year: %04d", FIX_YEAR + my_time[0]);
    OLED_Printf(0, 32, OLED_8X16,
                "Month: %02d", my_time[1]);
    OLED_Printf(0, 48, OLED_8X16,
                "Day: %02d", my_time[2]);
}

static void Show_SetTime(void)
{
    OLED_Clear();

    if (timeFlag <= 4)
    {
        Show_DateFields();

        OLED_ReverseArea(0, (timeFlag - 1) * 16, 128, 16);
    }
    else
    {
        OLED_Printf(0, 0, OLED_8X16,
                    "Hour: %02d", my_time[3]);
        OLED_Printf(0, 16, OLED_8X16,
                    "Minute: %02d", my_time[4]);
        OLED_Printf(0, 32, OLED_8X16,
                    "Second: %02d", my_time[5]);

        OLED_ReverseArea(0, (timeFlag - 5) * 16, 128, 16);
    }

    OLED_Update();
}

static void SetYear(void)
{
    while (1)
    {
        uint8_t key = Key_Event();

        if (key == 1)
        {
            if (my_time[0] < 99) my_time[0]++;
            else my_time[0] = 0;
        }
        else if (key == 2)
        {
            if (my_time[0] > 0) my_time[0]--;
            else my_time[0] = 99;
        }
        else if (key == 3)
        {
            return;
        }

        OLED_Clear();
        Show_DateFields();
        OLED_ReverseArea(48, 16, 32, 16);
        OLED_Update();
    }
}

static void SetTimePage(void)
{
    timeFlag = 1;

    while (1)
    {
        uint8_t key = Key_Event();

        if (key == 1)
        {
            if (timeFlag > 1) timeFlag--;
            else timeFlag = 7;
        }
        else if (key == 2)
        {
            if (timeFlag < 7) timeFlag++;
            else timeFlag = 1;
        }
        else if (key == 3)
        {
            if (timeFlag == 1)
            {
                return;
            }
            else if (timeFlag == 2)
            {
                SetYear();
            }
        }

        Show_SetTime();
    }
}

static void SettingPage(void)
{
    setFlag = 1;

    while (1)
    {
        uint8_t key = Key_Event();

        if (key == 1)
        {
            if (setFlag > 1) setFlag--;
            else setFlag = 2;
        }
        else if (key == 2)
        {
            if (setFlag < 2) setFlag++;
            else setFlag = 1;
        }
        else if (key == 3)
        {
            if (setFlag == 1)
            {
                return;
            }
            else
            {
                SetTimePage();
            }
        }

        Show_SettingPage_UI();
    }
}
```

## main 的调用结构

`SystemClock_Config()` 的声明、定义和外设初始化由 CubeMX 保留；只对照用户区域及主循环内容。

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_RTC_Init();
    MX_TIM2_Init();
    MX_I2C1_Init();

    /* USER CODE BEGIN 2 */
    OLED_Init();
    OLED_Clear();
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE END 2 */

    while (1)
    {
        /* USER CODE BEGIN 3 */
        uint8_t entry = First_Page_Clock();
        if (entry == 1)
        {
            /* 后续接入菜单应用。 */
        }
        else if (entry == 2)
        {
            SettingPage();
        }
        /* USER CODE END 3 */
    }
}
```

## 定时回调

放在 `USER CODE BEGIN 4`。中断入口仍由 `stm32f1xx_it.c` 调用 HAL 分发。

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        KEY_Scan();
    }
}
```

## 集成核对

- TIM2时钟8 MHz，PSC=7，ARR=999；NVIC已开启。
- I2C1启用，PB8/PB9重映射，生成 `i2c.h` 和 `i2c.c`。
- key.c、OLED.c、OLED_Data.c加入Keil工程。
- 中文使用UTF-8 with BOM，字库启用UTF-8并具备24×24分支。
- 不再保留初始化阶段的 `Entry:` 测试。
- 首页、设置页、字段页分别调用自己的绘图函数。
- 每个页面循环一次只领取一次按键事件。
- 当前确认年份仅保留RAM中的值，RTC写回与上电保留尚未接入。
