# 02 工程与编辑环境

[上一章](01-design.md) · [返回目录](../README.md) · [下一章：OLED](03-oled.md)

## 本章目标

建立可编译的 HAL 工程，并让 VS Code 能识别 `uint8_t`、`GPIOC` 和 HAL 函数。

## 1. CubeMX 配置

选择 STM32F103C8T6，至少完成以下配置：

| 项目 | 配置 |
|---|---|
| SYS → Debug | Serial Wire |
| SYSCLK | HSI，8 MHz |
| APB1 / APB2 | 分频为1 |
| PB1、PA6、PA4 | GPIO_Input，Pull-up |
| I2C1 | **启用 I2C 模式**，100 kHz，7位寻址 |
| I2C1引脚 | PB8=SCL、PB9=SDA，使用重映射 |
| TIM2 | Internal Clock，Prescaler=7，Counter Period=999 |
| TIM2 NVIC | 启用全局中断 |
| RTC | 若启用，选择LSE作为时钟，关闭RTC引脚输出 |
| RCC LSE | 与实物外部32.768 kHz晶振一致 |

在 Project Manager 中选择 MDK-ARM，并启用“按外设分别生成 `.c/.h` 文件”。保留用户代码区域。

**仅把 PB8/PB9 分配成 I2C1 信号还不够，还要启用 I2C1 外设模式。** 生成后应存在：

```text
Core/Inc/i2c.h
Core/Src/i2c.c
```

并能在 `main()` 中看到 `MX_I2C1_Init()`。

## 2. 理解1 ms定时参数

在当前8 MHz TIM2时钟下：

```text
更新频率 = 8,000,000 / (PSC + 1) / (ARR + 1)
         = 8,000,000 / 8 / 1000
         = 1000 Hz
更新间隔 = 1 ms
```

因此使用：

```c
htim2.Init.Prescaler = 7;
htim2.Init.Period = 999;
```

不要使用 `Period=0` 来计算1 ms。STM32F1参考手册说明，自动重装载值为0时计数会被阻止。[RM0008](https://www.st.com/resource/en/reference_manual/rm0008-stm32f103xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

## 3. Keil 工程

打开生成的 `.uvprojx`，检查：

- 芯片型号为 STM32F103C8。
- 预处理宏包含 `USE_HAL_DRIVER`、`STM32F103xB`。
- Include Paths 包含 `Core/Inc`、HAL、CMSIS 和自建的 `bsp` 目录。
- `bsp/key.c`、`bsp/OLED.c`、`bsp/OLED_Data.c` 已加入工程。

示例目录：

```text
Core/
  Inc/              # CubeMX生成的头文件
  Src/              # main.c、gpio.c、tim.c、i2c.c等
Drivers/            # HAL与CMSIS
bsp/
  key.c
  key.h
  OLED.c
  OLED.h
  OLED_Data.c
  OLED_Data.h
  OLED_HAL_Config.h
MDK-ARM/
```

头文件写在 Include Path 中，只解决查找头文件的问题；`.c` 源文件仍需要加入 Keil 编译列表。

## 4. VS Code 的代码提示

安装或启用 Microsoft **C/C++** 扩展，并用 VS Code 打开包含 `Core`、`Drivers`、`bsp` 的工程根目录。

创建 `.vscode/c_cpp_properties.json`：

```json
{
  "configurations": [
    {
      "name": "STM32F103C8",
      "compilerPath": "${env:USERPROFILE}/.eide/tools/gcc_arm/bin/arm-none-eabi-gcc.exe",
      "compilerArgs": ["-mcpu=cortex-m3", "-mthumb"],
      "intelliSenseMode": "windows-gcc-arm",
      "includePath": [
        "${workspaceFolder}/Core/Inc",
        "${workspaceFolder}/bsp",
        "${workspaceFolder}/Drivers/STM32F1xx_HAL_Driver/Inc",
        "${workspaceFolder}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy",
        "${workspaceFolder}/Drivers/CMSIS/Device/ST/STM32F1xx/Include",
        "${workspaceFolder}/Drivers/CMSIS/Include"
      ],
      "defines": ["USE_HAL_DRIVER", "STM32F103xB"],
      "cStandard": "c99",
      "cppStandard": "c++11"
    }
  ],
  "version": 4
}
```

这里的编译器路径对应 EIDE 安装的 ARM GCC。若你的安装位置不同，改成实际存在的 `arm-none-eabi-gcc.exe` 路径。它用于 VS Code 的交叉编译解析；工程构建继续按你的 Keil 配置执行。[微软交叉编译提示配置说明](https://code.visualstudio.com/docs/cpp/configure-intellisense-crosscompilation)

创建 `.vscode/settings.json`：

```json
{
  "C_Cpp.intelliSenseEngine": "default",
  "C_Cpp.autocomplete": "default",
  "C_Cpp.default.configurationProvider": "",
  "files.associations": {
    "*.c": "c",
    "*.h": "c"
  },
  "[c]": {
    "editor.quickSuggestions": {
      "other": true,
      "comments": false,
      "strings": false
    },
    "editor.suggestOnTriggerCharacters": true,
    "editor.parameterHints.enabled": true
  }
}
```

这个配置适用于本教程手工维护解析参数的方式。若另一个扩展正在提供当前文件的解析配置，应在当前工作区统一使用一种配置来源。

在 `key.h` 中包含 `main.h`，在 `key.c` 中包含 `key.h`：

```c
/* key.c */
#include "key.h"
```

配置完成后，执行命令面板中的 `Developer: Reload Window`，输入 `HAL_GPIO_` 并按 `Ctrl+Space`。仍无提示时可执行 `C/C++: Reset IntelliSense Database`，然后查看 `C/C++: Log Diagnostics` 中实际采用的路径和宏。

## 5. 中文编码

含中文字符串的 `main.c`、`OLED_Data.c` 和相关字库头文件统一使用 **UTF-8 with BOM**，并在 OLED 库中启用 UTF-8 分支。

Arm Compiler 5 会依据 BOM 识别 UTF-8；没有明确编码标记时，系统区域设置可能影响字符串解析。[Arm 编译器文档](https://documentation-service.arm.com/static/5eb946b50f1c1e0dae6ee21e)

不要只切换编辑器的显示编码而不重新保存文件。字库索引字节必须与主程序字符串一致。

## 验证

1. Keil 能编译空的 HAL 工程。
2. VS Code 中 `uint8_t`、`GPIOC`、`HAL_GPIO_ReadPin` 能跳转或补全。
3. `i2c.c/i2c.h` 和 `MX_I2C1_Init()` 均存在。
4. TIM2 配置为1 ms，但实际启动中断将在第05章完成。
