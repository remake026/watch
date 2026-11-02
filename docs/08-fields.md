# 08 日期时间字段选择

[上一章](07-settings.md) · [返回目录](../README.md) · [下一章：年份编辑](09-year.md)

## 本章目标

进入日期时间页，选择年、月、日、时、分、秒，按序号自动切换两屏，并能返回设置页。

## 1. 一份选中状态管理七个选项

全局用户变量区加入：

```c
uint8_t timeFlag = 1;
```

| 序号 | 字段 | 数据 |
|---|---|---|
| 1 | 返回 | — |
| 2 | 年 | `FIX_YEAR + my_time[0]` |
| 3 | 月 | `my_time[1]` |
| 4 | 日 | `my_time[2]` |
| 5 | 时 | `my_time[3]` |
| 6 | 分 | `my_time[4]` |
| 7 | 秒 | `my_time[5]` |

64像素高的屏幕每屏放四行16像素文字。沿用作者两屏布局：

```text
第一屏              第二屏
Y=0   < Back        Y=0   Hour
Y=16  Year          Y=16  Minute
Y=32  Month         Y=32  Second
Y=48  Day
```

## 2. 根据序号绘图

在 `SettingPage()` 前面新增以下绘图函数和交互函数：

```c
static void Show_SetTime(void)
{
    OLED_Clear();

    if (timeFlag <= 4)
    {
        OLED_ShowString(0, 0, "< Back", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16,
                    "Year: %04d", FIX_YEAR + my_time[0]);
        OLED_Printf(0, 32, OLED_8X16,
                    "Month: %02d", my_time[1]);
        OLED_Printf(0, 48, OLED_8X16,
                    "Day: %02d", my_time[2]);

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
```

序号1～4的反色Y坐标为 `(timeFlag-1)*16`；序号5～7为 `(timeFlag-5)*16`。选中行使用整行128像素反色。

## 3. 字段选择循环

```c
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
            else
            {
                /* 下一章接入年份编辑。 */
            }
        }

        Show_SetTime();
    }
}
```

这里末尾必须调用 **`Show_SetTime()`**。如果误写成 `Show_SettingPage_UI()`，变量虽然在变化，屏幕却一直显示上一级设置页，并可能出现“`Show_SetTime` 未使用”的编译警告。

## 4. 接入设置页

在 `SettingPage()` 中，确认第二项时调用：

```c
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
```

`SetTimePage()` 放在 `SettingPage()` 前面，就能直接调用；若采用其他排列，需要在函数使用前增加相应的 `static` 声明。

## 验证

1. 首页 → 设置 → Date / Time，出现日期字段。
2. 从日向下选择时，切到时分秒屏。
3. 从时向上选择时，切回日期屏。
4. 秒的下一项是返回；返回的上一项是秒。
5. 确认返回，回到设置页。

本章只完成字段选择，确认其他字段时还没有修改数值。
