# 03 OLED 与硬件 I2C

[上一章](02-environment.md) · [返回目录](../README.md) · [下一章：中文与字号](04-fonts.md)

## 本章目标

先让屏幕稳定显示 `Hello World!`。本章使用配套 OLED 图形库，替换其底层通信部分以适配 HAL 硬件 I2C。

## 1. 核对接线和模式

| OLED | STM32 |
|---|---|
| GND | GND |
| SCL | PB8 |
| SDA | PB9 |
| VCC | 按模块规格接供电 |

本章对应4针 I2C 屏。若模块为 SPI 接口，需要使用对应驱动。

STM32F103 的 I2C1 默认引脚是 PB6/PB7；PB8/PB9 需要重映射。CubeMX 应在 I2C1 MSP 初始化中生成类似代码：

```c
__HAL_RCC_GPIOB_CLK_ENABLE();

GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

__HAL_AFIO_REMAP_I2C1_ENABLE();
__HAL_RCC_I2C1_CLK_ENABLE();
```

AFIO 时钟由 HAL MSP 初始化开启。以 CubeMX 生成为主，避免把同一组引脚又配置成普通输出。

I2C 总线需要上拉；先检查模块是否已经带有上拉电阻和实际线路的高电平。不能用 GPIO 推挽高电平代替 I2C 上拉。

## 2. 接入图形库

从原配套工程复制 `OLED.c`、`OLED.h`、`OLED_Data.c`、`OLED_Data.h` 到 `bsp`，保留原有说明和署名，并加入 Keil 工程。保持字库原有编码，等第04章统一转换。

原作者用 GPIO 模拟 I2C。本教程改成 HAL 硬件 I2C 后，旧的 `OLED_GPIO_Init()`、`OLED_WriteCommand()`、`OLED_WriteData()` 等同名底层实现需要替换，不能保留两套重复定义。

新增 `OLED_HAL_Config.h`：

```c
#ifndef OLED_HAL_CONFIG_H
#define OLED_HAL_CONFIG_H

#include "i2c.h"

#define OLED_HAL_BUS           hi2c1
#define OLED_HAL_ADDRESS       (0x3CU << 1)
#define OLED_HAL_COLUMN_OFFSET 0U

extern volatile HAL_StatusTypeDef OLED_HAL_LastStatus;

void OLED_WriteCommand(uint8_t command);
void OLED_WriteData(uint8_t *data, uint8_t count);
void OLED_SetCursor(uint8_t page, uint8_t x);

#endif
```

这里的 `0x3C` 是7位设备地址。STM32 HAL 的这些 I2C 接口接收的是已经左移一位的地址。若实物响应地址为 `0x3D`，对应修改宏。

在 `OLED.c` 中包含：

```c
#include "main.h"
#include "OLED.h"
#include "OLED_HAL_Config.h"
#include <string.h>
```

移除原标准库设备头引用 `stm32f10x.h`，其余绘图所需的标准头继续保留。

底层发送函数：

```c
volatile HAL_StatusTypeDef OLED_HAL_LastStatus = HAL_OK;

void OLED_GPIO_Init(void)
{
    /* I2C引脚由MX_I2C1_Init配置，此处等待屏幕供电稳定。 */
    HAL_Delay(100);
}

void OLED_WriteCommand(uint8_t command)
{
    uint8_t packet[2] = {0x00, command};

    OLED_HAL_LastStatus = HAL_I2C_Master_Transmit(
        &OLED_HAL_BUS, OLED_HAL_ADDRESS,
        packet, sizeof(packet), 20);
}

void OLED_WriteData(uint8_t *data, uint8_t count)
{
    uint8_t packet[129];

    if (count > 128)
    {
        OLED_HAL_LastStatus = HAL_ERROR;
        return;
    }

    packet[0] = 0x40;
    memcpy(packet + 1, data, count);

    OLED_HAL_LastStatus = HAL_I2C_Master_Transmit(
        &OLED_HAL_BUS, OLED_HAL_ADDRESS,
        packet, (uint16_t)count + 1U, 30);
}
```

`0x00` 表示后面是命令，`0x40` 表示后面是显示数据。这两个控制字节对应本配套库使用的 I2C 显示控制方式。

游标函数沿用原库的页寻址方式：

```c
void OLED_SetCursor(uint8_t page, uint8_t x)
{
    x += OLED_HAL_COLUMN_OFFSET;
    OLED_WriteCommand(0xB0 | page);
    OLED_WriteCommand(0x10 | ((x & 0xF0) >> 4));
    OLED_WriteCommand(x & 0x0F);
}
```

列偏移要与屏幕控制器和面板连接匹配。配套库对部分 SH1106 模块使用偏移2；不要只看“1.3寸”就断定控制器型号。初始化命令也应与实物模块匹配。

## 3. 第一次显示

`main.c` 用户包含区加入：

```c
#include "OLED.h"
```

在 `MX_I2C1_Init()` 等外设初始化之后调用：

```c
OLED_Init();
OLED_Clear();
OLED_ShowString(0, 0, "Hello World!", OLED_6X8);
OLED_Update();
```

库的工作方式是：

```text
OLED_Clear / ShowString / Printf / ReverseArea
          ↓ 修改RAM中的OLED_DisplayBuf
OLED_Update
          ↓ 通过I2C发送显存
屏幕变化
```

因此只调用 `OLED_Clear()` 或显示函数，不一定会立刻改变屏幕。

## 验证与排查

1. 屏幕左上角出现 `Hello World!`。
2. 在最后一次发送后观察 `OLED_HAL_LastStatus`，正常应为 `HAL_OK`。它只记录最近一次发送结果，不能证明所有初始化命令都成功。
3. 黑屏时先在 `OLED_Init()` 打断点，确认程序能执行到这里。
4. 若到不了，检查前面的时钟和 RTC 初始化；本工程 LSE 不起振时会进入错误处理。
5. 若能到达，再检查总线接线、地址、上拉和模块型号。

可在 I2C 初始化、屏幕供电稳定后临时检查设备应答：

```c
HAL_StatusTypeDef oled_ready;
oled_ready = HAL_I2C_IsDeviceReady(&hi2c1, 0x3C << 1, 3, 100);
```

`HAL_OK` 表示该地址有设备应答，仍需结合初始化和显示结果确认屏幕工作。
