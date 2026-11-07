# 应用与页面模块

`app.c` 负责OLED、按键扫描定时器的启动，以及首页返回后的页面调度。页面沿用原来的阻塞循环：进入 `*_Run()`，按返回键后执行 `return`，回到调用者。

| 文件 | 对外接口 | 职责 |
|---|---|---|
| `app.c` | `App_Init()`、`App_Run()` | 启动应用、保存共享时间缓存、调度首页/菜单/设置 |
| `home_page.c` | `HomePage_Run(time)` | 从RTC更新时间缓存，显示首页，返回入口编号1或2 |
| `settings_page.c` | `SettingsPage_Run(time)` | 字段选择和RAM中的日期时间编辑 |
| `menu_page.c` | `MenuPage_Run()` | 菜单滑动、子页面调用、PB15切换、返回后恢复菜单 |
| `stopwatch_page.c` | `StopwatchPage_Run()` | 秒表计时、选择、开始、暂停和清零 |
| `game_page.c` | `GamePage_Run()` | 游戏输入、固定步长循环和局部刷新 |
| `game.c` | `Game_Step()`等 | 独立的运动、碰撞、计分规则 |
| `animation_page.c` | `AnimationPage_Run()` | KUN动画播放与退出 |
| `oled_test_page.c` | `OLEDTestPage_Run()` | OLED显示时钟参数测试 |

各模块的绘图辅助函数和页面独有状态保持 `static`，头文件只公开页面入口。首页和设置通过参数接收同一份六字节时间数组，依次为年份偏移、月、日、时、分、秒，数组由 `app.c` 持有。调用者应提供生命周期覆盖整个页面调用的有效数组。

OLED、按键、RTC驱动保留原来的接口；首页通过 `rtc.h` 调用已有的 `RTC_ReadDateTime()`。本次迁移保留原有编辑范围、RTC写回情况、秒表退出后的状态和菜单编号，功能修正应另行实施。

Keil工程已在 `bsp` 分组中注册全部页面源文件。包含中文的页面源文件使用UTF-8 BOM，以便Arm Compiler 5正确识别中文字符串。

## 验证

在项目根目录执行，需要PATH中存在GCC：

```powershell
./tests/run_game_tests.ps1
./tests/run_page_tests.ps1
```

游戏测试覆盖规则和显存一致性，绘图代码从 `game_page.c` 提取。页面测试直接编译各页面源码，使用主机HAL/OLED替身检查首页选择、六字段编辑、秒表暂停清零及重新进入、PB15切换、动画、OLED参数测试、游戏入口和返回路径。

迁移验收时，页面测试还与迁移前 `app.c` 的九组按键、时间和绘图调用轨迹逐项比较，结果一致。`run_page_tests.ps1 -ReferenceAppPath <原app.c路径>` 可对保留的原文件再次比较。主机测试不代替板上验收。
