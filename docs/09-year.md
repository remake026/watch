# 09 年份编辑

[上一章](08-fields.md) · [返回目录](../README.md) · [整合代码](appendix-a-integration.md)

## 本章目标

选择 Year 后确认进入编辑；键1加一年，键2减一年，键3确认返回字段选择页。

本章完成 RAM 数组的编辑和页面返回。原作者在修改数组后会写回 RTC；HAL RTC 读写和上电保留将在后续单独实现，因此本章不能宣称已经保存到硬件时钟。

## 1. 选择与编辑拥有不同的按键含义

| 当前函数 | 键1 | 键2 | 键3 |
|---|---|---|---|
| `SetTimePage()` | 上一个字段 | 下一个字段 | 进入编辑 / 返回 |
| `SetYear()` | 年份加1 | 年份减1 | 确认返回 |

进入 `SetYear()` 后，由它消费按键事件。`SetTimePage()` 暂停在调用处，等子函数返回后继续运行。

## 2. 提取可复用的日期绘图内容

字段选择时整行反色，数值编辑时只给数字反色。先提取两者共有的内容绘制。

在 `Show_SetTime()` 前面增加：

```c
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
```

把 `Show_SetTime()` 的第一屏分支改成：

```c
if (timeFlag <= 4)
{
    Show_DateFields();
    OLED_ReverseArea(0, (timeFlag - 1) * 16, 128, 16);
}
```

保留函数原有开头的清屏、后面的时间屏分支和末尾的更新。

`Show_DateFields()` 只绘制内容；调用者决定清屏、反色和统一刷新。不要在这个公共函数中提前 `OLED_Update()`。

## 3. 年份范围和无符号边界

`my_time[0]` 为 `uint8_t`。本阶段限定0～99，表示2000～2099年，以便后续衔接 HAL 日期接口的年份字段。

不能从0直接减1后再判断是否小于0：无符号值会变成255。正确做法是先判断边界，再修改。

## 4. 编写年份编辑函数

放在 `Show_SetTime()` 后、`SetTimePage()` 前：

```c
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
```

`"Year: "` 有6个ASCII字符，每个宽8像素，所以年份数字从X=48开始。四位年份总宽32像素。这就是反色区域 `(48,16,32,16)` 的来源。

`SetYear()` 只改变数组，不改变 `timeFlag`。返回后，外层选择仍停留在 Year。

## 5. 接入字段页

用下面内容替换 `SetTimePage()` 中的键3处理：

```c
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
```

其他字段后续逐项接入，不要先写尚未定义的 `SetMonth()` 等调用。

## 验证

1. 选中 Year，确认后只有年份数字反色。
2. 键1把2026改成2027，键2把它改回2026。
3. 确认后回到字段选择，整行 Year 反色。
4. 返回首页，看到同一个数组中的新年份。
5. 2000减一年变2099；2099加一年变2000。

复位后，数组仍会恢复固件中定义的初值。这是当前阶段的数据范围，不是 RTC 保存成功的标志。

## 后续接 RTC 前需要解决的问题

- 从实际 RTC 读取当前日期时间，避免缓存数组与 RTC 原始日期不一致。
- 修改月份、年份时校验日期，例如闰年2月29日。
- 区分普通整数 `RTC_FORMAT_BIN` 与 BCD 编码。
- 日期和时间写回后再统一刷新缓存。
- STM32F1 HAL 软件日历及备份域的保存策略。
- 避免每次启动无条件调用生成的设定时间代码，覆盖已经设置的值。

这些内容作为后续功能逐项完成。
