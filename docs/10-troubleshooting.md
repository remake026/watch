# 10 排错记录

[返回目录](../README.md) · [整合代码](appendix-a-integration.md)

## 1. GPIOC处提示 expected a type specifier

先检查函数调用的位置。下面的语句不能直接写在函数外：

```c
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
```

`USER CODE BEGIN 0` 本身位于文件作用域，适合放函数定义，不代表其中任何位置都能写执行语句。需要执行的 GPIO 调用放在 `main()` 初始化之后或其他函数体内。

`GPIO_PIN_RESET` 表示输出低电平，`GPIO_PIN_SET` 表示高电平。注释要与实际参数一致。

## 2. PC13灯一直亮

先根据原理图判断 LED 是否低电平有效。常见最小系统板采用电源→电阻/LED→PC13的接法，PC13拉低时点亮。

再检查 PC13 是否仍被 RTC 输出占用：

```c
hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
```

CubeMX 中同步关闭 RTC 输出，把 PC13 配置为普通 GPIO 输出。芯片有 GPIOC 的定义，并不等于引脚已经配置成预期用途。

## 3. OLED接PB8/PB9，代码却是PB6/PB7

检查 I2C1 MSP 初始化中的引脚和 `__HAL_AFIO_REMAP_I2C1_ENABLE()`。同时修改 CubeMX 配置，避免下次生成又覆盖。

## 4. cannot open source input file i2c.h

按顺序检查：

1. `Core/Inc/i2c.h` 是否真的存在。
2. CubeMX 中 I2C1 的 I2C 模式是否启用。
3. 是否启用了“按外设分别生成文件”。
4. 是否生成 `i2c.c`、`MX_I2C1_Init()` 和 `hi2c1`。
5. HAL I2C模块和源文件是否参与编译。
6. 最后再检查 Include Path。

如果模式未开启，仅修补头文件搜索路径不能恢复缺失的外设驱动。

## 5. OLED黑屏

先确认程序能执行到 `OLED_Init()`，再检查电气和总线。LSE、RTC 或其他前置初始化失败，都可能使程序在屏幕初始化前进入 `Error_Handler()`。

若能执行到显示函数，检查地址应答和 `OLED_HAL_LastStatus`，再核对控制器型号、初始化命令及列偏移。

## 6. 中文不显示或乱码

- 主程序和字库索引是否使用相同编码？
- 使用Arm Compiler 5时，UTF-8文件是否有BOM？
- 是否只启用了一个字符集宏？
- 所选字号的中文查表分支是否存在？
- 对应字是否加入了该字号的字模表？
- 表尾是否保留了空索引的占位图？

只有16×16字模不会自动得到24×24字模。

## 7. Flash Timeout. Reset the Target and try it again

这是 Flash 操作超时，需要结合下载连接和目标状态排查，不能仅凭这个提示判断芯片损坏。[Keil 错误说明](https://www.keil.com/support/man/docs/ulinkpro/ulinkpro_errors.asp)

先退出调试，重新供电并复位，再下载。仍失败时：

- 检查供电、共地、SWDIO、SWCLK是否可靠。
- 在调试器设置中确认使用SW接口，降低SWD频率。
- 核对芯片型号和对应的Flash算法。
- 需要复位下连接时，将探针NRST接到目标NRST，再选择相应的连接/硬件复位方式。

复位下连接的具体支持以所用 ST-LINK 和软件版本为准。[ST-LINK说明](https://www.st.com/resource/en/user_manual/cd00262073-stm32-stlink-utility-software-description-stmicroelectronics.pdf)

## 8. key.c里没有代码提示

Keil 的工程成员和 VS Code 的 IntelliSense 配置是两个检查点。

在 VS Code 中先看：

- 当前文件语言是否为C。
- C/C++扩展是否在当前配置文件和工作区启用。
- `key.c → key.h → main.h → HAL` 的包含链是否完整。
- ARM编译器路径、芯片宏和Include Paths是否存在。
- 当前文件实际由哪个扩展或配置提供解析参数。

然后重新加载窗口，输入 `HAL_GPIO_` 触发补全。参见[第02章](02-environment.md)。

## 9. 按键没有反应或偶尔漏键

| 检查点 | 常见问题 |
|---|---|
| 定时器启动 | 只有初始化，没有 `HAL_TIM_Base_Start_IT()` |
| 定时周期 | 7999与999组合在8 MHz下是1秒；ARR=0又会阻止计数 |
| 回调函数 | 调用了取事件函数，却没有调用扫描函数 |
| 采样位置 | 每1ms更新前后状态，却每20次才判断边沿 |
| 页面消费 | 同一循环调用多次 `Key_Event()`，前一次已清空事件 |
| 用户预期 | 当前方案在松手触发，按住不会连发 |

当前单槽事件也不能保证保存处理不及时期间的所有连续按键。扩展复杂交互时，再设计事件队列。

## 10. 进入字段页后仍显示Date / Time

`SetTimePage()` 末尾应调用 `Show_SetTime()`。上一级 `SettingPage()` 末尾才调用 `Show_SettingPage_UI()`。

“函数已定义但未被使用”的警告值得检查：它可能说明对应功能从未真正接入调用链。

## 11. 返回首页残留文字或反色异常

首页每次绘制遵循：清空显存、绘制正常内容、给当前区域反色、统一更新。不要对未重绘的区域连续取反。

24×24中文的两个字宽48像素，右对齐起点是80。文字和反色区域使用相同的起点。

## 12. 首次确认只显示Entry编号

检查初始化区域是否还保留第06章的临时 `Entry:` 测试。连接页面后，首页只由主循环调用。

## 13. 年份显示改变，但时间不走或复位恢复

当前阶段只编辑 RAM 数组。重绘次数增加不会让数组中的秒数自行增加；确认返回也没有自动调用 RTC 接口。后续接 RTC 同步和备份保存后，再验证真实走时与上电保留。
