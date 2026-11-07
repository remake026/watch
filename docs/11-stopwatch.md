# 11 接着当前工程：秒表

[返回目录](../README.md)

本课以当前 `bsp/app.c` 为起点，按“设计原因 → 代码位置 → 编码步骤 → 验证效果 → 常见错误”推进。下面是供你手工编写的教学代码，尚未写入固件源文件。

## 1. 设计原因

你已经写出首页、日期时间数组编辑、滑动菜单，以及菜单下标2确认后切换PB15。现在补上菜单下标1的秒表。

参考 [BQT0411/STM32-WATCH 的配套源码](https://github.com/BQT0411/STM32-WATCH/blob/main/Keil%E5%AE%8C%E6%95%B4%E4%BB%A3%E7%A0%81.zip) 中 `Hardware/menu.c` 的秒表部分，保留四个选项：返回、开始、暂停、清零。键1上一项，键2下一项，键3执行选中项。

原版通过定时器周期调用 `StopWatch_Tick()` 累计时间。本课用已有的 `HAL_GetTick()` 获取毫秒时基，在页面中累计实际经过时间，先掌握秒表状态和页面调用。`HAL_GetTick()` 由当前工程的SysTick中断维护，不需要新增定时器。

区分三个状态：

| 变量 | 含义 |
|---|---|
| `selection` | 光标选中了哪个操作 |
| `running` | 是否正在计时 |
| `elapsedMs` | 已经累计了多少毫秒 |

移动光标只修改 `selection`；按确认才修改 `running` 或清零。开始后移到暂停项，不会立刻暂停。

每次循环计算 `now - lastTick`，运行时才把差值加进累计时间；暂停时仍更新 `lastTick`，这样恢复时不会把暂停期间加进去。OLED刷新需要时间，不能把页面每循环一次当作经过1ms。

本课约定：清零同时暂停；退出后不在后台计时；重新进入从零开始；显示到99:59:59后回到00:00:00。

## 2. 代码位置

只在 `bsp/app.c` 中手工添加：

1. 在 `static void ShowMenu(int32_t position)` 前面，依次放入下面的绘图函数和秒表页面函数。
2. 在 `Menu()` 的键3分支中，增加 `menuFlag == 1` 的处理。

当前文件已经包含 `main.h`、`OLED.h`、`key.h`，足够使用下面的接口。TIM2继续负责原有按键扫描，本课不改中断。

## 3. 编码步骤

先加入绘图函数。它只显示状态，不处理按键、不推进时间：

```c
static void Show_StopWatch_UI(uint32_t elapsedMs,
                              uint8_t selection,
                              uint8_t running)
{
    uint32_t totalSeconds = elapsedMs / 1000U;

    OLED_Clear();
    OLED_ShowString(0, 0, "< Back", OLED_8X16);
    OLED_ShowString(80, 0, running ? "RUN" : "STOP", OLED_8X16);
    OLED_Printf(16, 20, OLED_12X24, "%02u:%02u:%02u",
                (unsigned int)(totalSeconds / 3600U),
                (unsigned int)((totalSeconds / 60U) % 60U),
                (unsigned int)(totalSeconds % 60U));

    OLED_ShowString(0, 48, "Start", OLED_8X16);
    OLED_ShowString(48, 48, "Stop", OLED_8X16);
    OLED_ShowString(96, 48, "Clr", OLED_8X16);

    if (selection == 1) OLED_ReverseArea(0, 0, 48, 16);
    else if (selection == 2) OLED_ReverseArea(0, 48, 40, 16);
    else if (selection == 3) OLED_ReverseArea(48, 48, 32, 16);
    else if (selection == 4) OLED_ReverseArea(96, 48, 24, 16);

    OLED_Update();
}
```

时间格式是“时:分:秒”。8个字符每个宽12像素，共96像素，从X=16开始正好居中。底部反色宽度来自字符数乘8。

紧接着加入秒表页面：

```c
static void StopWatch(void)
{
    uint8_t selection = 1;
    uint8_t running = 0;
    uint32_t elapsedMs = 0;
    uint32_t lastTick = HAL_GetTick();

    while (1)
    {
        uint32_t now = HAL_GetTick();
        uint8_t key = Key_Event();

        if (running)
        {
            elapsedMs = (elapsedMs + (now - lastTick)) % 360000000UL;
        }
        lastTick = now;

        if (key == 1)
        {
            if (selection > 1) selection--;
            else selection = 4;
        }
        else if (key == 2)
        {
            if (selection < 4) selection++;
            else selection = 1;
        }
        else if (key == 3)
        {
            if (selection == 1) return;
            else if (selection == 2) running = 1;
            else if (selection == 3) running = 0;
            else if (selection == 4)
            {
                running = 0;
                elapsedMs = 0;
            }
        }

        Show_StopWatch_UI(elapsedMs, selection, running);
    }
}
```

`360000000` 是100小时的毫秒数。差值使用无符号减法，在相邻读取间隔小于一次32位计数周期时，也能跨过 `HAL_GetTick()` 回绕。暂停时保留不足1秒的部分，恢复后继续累计。

最后，在 `Menu()` 里现有的 `menuFlag == 0` 返回分支之后、`menuFlag == 2` 灯控制分支之前，插入：

```c
else if (menuFlag == 1)
{
    StopWatch();

    /* Restore the entire menu after the child page returns. */
    ShowMenu(position);
    OLED_Update();
}
```

调用关系为 `App_Run() → Menu() → StopWatch()`。秒表执行 `return` 后，先回到 `Menu()` 的调用处，再完整重画菜单。

这里必须完整刷新：你的 `ShowMenu()` 最后只调用 `OLED_UpdateArea(0,16,128,32)`，更新Y=16～47。秒表在Y=0和Y=48画过文字，仅局部刷新会留下文字残影。`ShowMenu()` 先在RAM里清屏和绘图，随后 `OLED_Update()` 把整幅菜单送到屏幕。

## 4. 验证效果

文档检查记录：已将上述三个代码块临时合入当前 `app.c` 的副本，使用本机Arm Compiler 5.06和本工程HAL/CMSIS头文件编译成目标文件，结果为0错误，保留原文件已有的2条警告（无符号数与0比较、`timeFlag`未使用）。这项检查覆盖接口和语法，未进行整工程链接、下载或板上验收；原固件源文件保持原样。

按顺序在Keil编译、下载，再逐项检查：

| 操作 | 预期结果 |
|---|---|
| 菜单选中秒表并确认 | 显示00:00:00、STOP，光标在Back |
| 键2选Start，键3确认 | 显示RUN，时间开始增加 |
| 移到Stop但不确认 | 时间继续增加 |
| 在Stop上确认 | 显示STOP，时间停止增加 |
| 暂停等待5秒，再确认Start | 从原累计值继续，未加上暂停的5秒 |
| 计时中选Clr并确认 | 显示00:00:00和STOP |
| Back确认 | 回到秒表图标，顶部和底部没有残留文字 |
| 再次进入秒表 | 从00:00:00暂停状态开始 |

用手机秒表对照运行约60秒，检查没有因OLED刷新产生明显的累计变慢。本工程使用HSI，显示和按键也有响应延迟，这一步用于验证累计逻辑，不代表已经完成精密计时校准。

## 5. 常见错误

- 每次循环写 `elapsedMs++`：页面循环不是固定1ms，显示越慢计时越慢。
- 只在运行时更新 `lastTick`：恢复时会把暂停的时间一起计入。
- 选中Start就开始：把光标状态和运行状态混在一起，应在键3分支执行操作。
- 秒表返回后屏幕残留Start等文字：漏了菜单恢复后的完整 `OLED_Update()`。
- 调用 `StopWatch()` 编译报隐式声明或类型冲突：把定义放在 `Menu()` 前，或提前声明。

当前工程还有几项后续需要单独处理：`my_time` 仍为RAM数组，首页尚未读取RTC；`SetTime()` 中无符号数小于0的判断无效，小时也不能统一限制为59；年月日还缺合法日期校验。先完成并验收本课，再逐项修正。
