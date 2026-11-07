# 当前源码对照快照

[模块目录](../README.md)

这些文件从2026-11-07本地工程复制，供分步编码后核对。**它们不是可独立构建的完整固件，不要把此目录作为第二份源码加入Keil。** 含中文文件的原始编码与BOM保持不变，文件哈希见 [manifest.json](manifest.json)。

| 教学用途 | 对照文件 |
|---|---|
| 外设配置与入口 | [main.c](Core/Src/main.c)、[gpio.c](Core/Src/gpio.c)、[i2c.c](Core/Src/i2c.c)、[tim.c](Core/Src/tim.c) |
| RTC读取/初始化 | [rtc.c](Core/Src/rtc.c)、[rtc.h](Core/Inc/rtc.h) |
| 中断入口 | [stm32f1xx_it.c](Core/Src/stm32f1xx_it.c) |
| 显示适配声明 | [OLED.h](bsp/OLED.h)、[OLED_Data.h](bsp/OLED_Data.h)、[OLED_HAL_Config.h](bsp/OLED_HAL_Config.h)、[watch_config.h](bsp/watch_config.h) |
| 按键 | [key.c](bsp/key.c)、[key.h](bsp/key.h) |
| 应用调度 | [app.c](bsp/app.c)、[app.h](bsp/app.h) |
| 首页 | [home_page.c](bsp/home_page.c)、[home_page.h](bsp/home_page.h) |
| 设置 | [settings_page.c](bsp/settings_page.c)、[settings_page.h](bsp/settings_page.h) |
| 菜单 | [menu_page.c](bsp/menu_page.c)、[menu_page.h](bsp/menu_page.h) |
| 秒表 | [stopwatch_page.c](bsp/stopwatch_page.c)、[stopwatch_page.h](bsp/stopwatch_page.h) |
| 游戏规则 | [game.c](bsp/game.c)、[game.h](bsp/game.h) |
| 游戏页面 | [game_page.c](bsp/game_page.c)、[game_page.h](bsp/game_page.h) |
| 动画页面 | [animation_page.c](bsp/animation_page.c)、[animation_page.h](bsp/animation_page.h) |
| 显示时钟测试 | [oled_test_page.c](bsp/oled_test_page.c)、[oled_test_page.h](bsp/oled_test_page.h) |

完整OLED图形库、原始字库/压缩数组、HAL/CMSIS、其余生成头文件和工程配置已包含在仓库的 [firmware目录](../../../firmware/README.md)，不要用本快照替代完整工程。通信适配代码见[模块02](../02-oled.md)，解码与播放见[模块14](../14-animation.md)。生成文件保留ST原有说明，OLED声明保留江协科技原署名，素材来源沿用各章节的记录。

快照保留当前缺陷以便与真实工程一致，例如 `uint8_t < 0`无效判断、RTC每次默认初始化、秒表单秒增量。不要把“与快照相同”理解为“已完成所有改进”。改进代码要在独立步骤中设计、测试并更新说明。
