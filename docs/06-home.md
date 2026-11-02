# 06 首页选择

[上一章](05-keys.md) · [返回目录](../README.md) · [下一章：设置页](07-settings.md)

## 本章目标

首页显示日期、时间和“菜单 / 设置”。键1上一项，键2下一项，键3返回当前选中项。

## 1. 页面状态和数据

在 `USER CODE BEGIN PD` 中定义：

```c
#define FIX_YEAR 2000
```

在 `USER CODE BEGIN PV` 中定义一次：

```c
uint8_t my_time[] = {26, 11, 2, 21, 13, 0};
uint8_t cliFlag = 1;
```

`cliFlag=1` 表示菜单，`cliFlag=2` 表示设置。这是选项编号，不是按键编号。

当前日期时间来自固定数组；本章的目标是完成页面交互，RTC 同步另行实现。

## 2. 绘制首页

放在 `USER CODE BEGIN 0` 中，`main()` 前面：

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
```

绘制顺序必须稳定：

```text
清空显存 → 正常内容 → 当前选项反色 → 更新屏幕
```

取反函数会翻转像素。每次先重画正常页面，再取反一次，可以避免背景反复翻转和子页面残留。

## 3. 读取按键并返回入口

放在绘图函数后面：

```c
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
```

保持 `cliFlag` 在1～2之间，避免无符号减法下溢。只有两个选项时，上一项和下一项看起来都会左右切换；增加选项后，两者的方向区别会体现出来。

## 4. 临时验证返回值

本章可以在 OLED 和 TIM2 启动后临时执行：

```c
uint8_t entry = First_Page_Clock();

OLED_Clear();
OLED_Printf(0, 0, OLED_8X16, "Entry: %d", entry);
OLED_Update();
```

此时主循环留空。选择菜单确认显示 `Entry: 1`；选择设置确认显示 `Entry: 2`。

这个测试只执行一次首页。**下一章连接页面时，移除这段初始化阶段的测试，改由主循环调用首页。** 否则第一次确认会先被测试入口接住。

## 验证

- 上电默认菜单反色。
- 键1、键2松开后切换选项。
- 反色区域与文字对齐，“设置”右侧没有裁切。
- 确认返回1或2。
