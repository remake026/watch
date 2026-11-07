# 01 工程、时钟与配置

[模块目录](README.md) · [上一章](00-architecture.md) · [下一章](02-oled.md)

## 设计思想

先建立能启动的工程，再加驱动。时钟决定TIM2参数，GPIO配置决定按键有效电平，I2C引脚与驱动宏必须一致。把生成代码留给CubeMX，把应用逻辑放在 `bsp` 和 `USER CODE` 区域。

## 变量与逻辑

| 名称 | 用途 | 当前值/约束 |
|---|---|---|
| `hi2c1` | OLED总线句柄 | 100kHz，PB8/PB9重映射 |
| `htim2` | 按键扫描定时器 | 8MHz / 8 / 1000 = 1kHz |
| `hrtc` | RTC句柄 | LSE，自动1秒预分频 |
| `WATCH_OLED_ADDRESS` | HAL设备地址 | `0x3C << 1` |
| `WATCH_OLED_COLUMN_OFFSET` | 显示列偏移 | 当前0，按实物适配 |
| `WATCH_OLED_DISPLAY_CLOCK` | 启动D5参数 | 当前 `0xF0`，候选值需实测 |

启动顺序是 `HAL_Init → SystemClock_Config → GPIO/RTC/TIM2/I2C1初始化 → App_Init → 主循环App_Run`。LSE初始化失败会进入 `Error_Handler`，此时OLED尚未启动，黑屏不能直接归因于显示驱动。

## A 移植：生成最小硬件工程

1. 选STM32F103C8T6，SYS选择Serial Wire；HSI为系统8MHz时钟，AHB/APB1/APB2均不分频。
2. PB1、PA6、PA4设上拉输入；PB15推挽输出且初始低；PC13按原板配置输出。
3. I2C1设100kHz、7位地址，PB8/PB9使用复用开漏并启用重映射；检查总线上拉。
4. TIM2使用内部时钟，PSC=7，ARR=999，打开全局中断。这里只初始化，之后由 `App_Init`启动。
5. RTC使用LSE，实物须具备对应晶振；生成MDK-ARM项目和按外设拆分的 `.c/.h`。
6. Keil加入 `bsp`包含路径，宏为 `USE_HAL_DRIVER`、`STM32F103xB`。不复制原标准库的初始化函数。

配置详细操作可查[原环境章](../02-environment.md)。当前值对照：[tim.c](reference/Core/Src/tim.c)、[i2c.c](reference/Core/Src/i2c.c)、[gpio.c](reference/Core/Src/gpio.c)。

## B 自己编码：配置边界与用户入口

### 第1步：写显示适配配置

在 `bsp/watch_config.h`定义当前实际使用的显示参数，在 `OLED_HAL_Config.h`映射它们：

```c
#define WATCH_OLED_I2C_HANDLE    hi2c1
#define WATCH_OLED_ADDRESS       (0x3CU << 1)
#define WATCH_OLED_COLUMN_OFFSET 0U
#define WATCH_OLED_DISPLAY_CLOCK 0xF0U
```

### 第2步：核对“定义宏”和“实际使用”

当前 `key.c`和PB15菜单分支仍直接写引脚，修改 `WATCH_KEY1_PIN`或 `WATCH_LED_PIN`不会自动改变它们。`WATCH_LESSON`尚未用于切换课程；`hi2c2/hadc1`宏也没有建立对应外设或应用。先沿用实物接线，统一宏引用作为独立改进。

### 第3步：为应用预留入口

先让CubeMX空工程编译通过；到应用章再在 `USER CODE BEGIN Includes`添加 `app.h`，在 `USER CODE BEGIN 2`调用 `App_Init`，在主循环调用 `App_Run`。不要现在调用尚未定义的函数。

## 验收与边界

单步运行到主循环；核对TIM2 PSC/ARR及I2C重映射；重新生成一次确认用户区域仍在。源码对照：[main.c](reference/Core/Src/main.c)、[watch_config.h](reference/bsp/watch_config.h)。生成代码里重复的 `app.h`包含不是需要照抄的教学步骤。
