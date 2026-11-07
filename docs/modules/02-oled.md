# 02 OLED显示：移植图形库，自己写通信适配

[模块目录](README.md) · [上一章](01-platform.md) · [下一章](03-assets.md)

## 设计思想

屏幕显示分两步：先在RAM画好，再通过I2C发出去。保留图形库的字符、线条和位图算法，把与原标准库/模拟I2C相关的部分换成HAL。页面只调用 `OLED_ShowString`等接口，不直接组织I2C报文。

## 变量与逻辑

| 变量 | 作用 | 生命周期/单位 |
|---|---|---|
| `OLED_DisplayBuf[8][128]` | 128×64显存 | 全局1024字节，每页8行 |
| `packet[129]` | 数据控制字+最多128字节 | 一次发送的栈缓冲 |
| `OLED_HAL_LastStatus` | 最近一次发送结果 | `volatile HAL_StatusTypeDef` |
| `OLED_HAL_BUS` | 适配总线句柄 | 映射 `hi2c1` |
| `OLED_HAL_ADDRESS` | HAL地址参数 | 7位地址左移1位 |

命令报文为 `[0x00, 命令]`，显存报文为 `[0x40, 数据…]`。`OLED_Clear/ShowImage/Printf`修改显存；`OLED_Update`发送全屏，`OLED_UpdateArea`发送相关页。只画不发，屏幕不会变。

## A 移植：把成熟图形库接进来

1. 从原配套资料取得 `OLED.c/.h`、`OLED_Data.c/.h`，放进 `bsp`，保留江协科技说明和署名。
2. 在Keil加入两个 `.c`文件，头文件通过包含路径查找。不能只复制文件而不注册编译。
3. 移除旧标准库设备头与旧模拟I2C实现，避免同名 `OLED_WriteCommand/Data`重复定义；保留图形算法和所需标准头。
4. 新建 [OLED_HAL_Config.h](reference/bsp/OLED_HAL_Config.h)，从 `watch_config.h`取得总线、地址、列偏移。
5. 芯片初始化序列、列偏移按实物模块确认；不能凭屏幕尺寸认定控制器。

完整移植操作与游标函数见[OLED基础章](../03-oled.md)。这一部分是库移植，不要求从零重写画线算法。

## B 自己编码：逐个写HAL适配函数

### 第1步：写命令发送

替换 `OLED.c`内旧同名函数，当前代码的超时为10ms：

```c
void OLED_WriteCommand(uint8_t command)
{
    uint8_t data[2] = {0x00, command};
    OLED_HAL_LastStatus = HAL_I2C_Master_Transmit(
        &OLED_HAL_BUS, OLED_HAL_ADDRESS, data, 2U, 10U);
}
```

先在文件中定义 `volatile HAL_StatusTypeDef OLED_HAL_LastStatus = HAL_OK;`，并包含 `main.h`、`OLED.h`、`OLED_HAL_Config.h`与 `string.h`。

### 第2步：写一页数据发送

```c
void OLED_WriteData(uint8_t *data, uint8_t count)
{
    uint8_t packet[129];
    if (count > 128U) {
        OLED_HAL_LastStatus = HAL_ERROR;
        return;
    }
    packet[0] = 0x40;
    memcpy(packet + 1, data, count);
    OLED_HAL_LastStatus = HAL_I2C_Master_Transmit(
        &OLED_HAL_BUS, OLED_HAL_ADDRESS,
        packet, (uint16_t)count + 1U, 20U);
}
```

边界检查保证不会向129字节栈缓冲复制过多数据。当前同步发送在函数返回前使用完缓冲；如果后续改DMA，必须重新设计缓冲生命周期。

### 第3步：完成初始化与游标

`OLED_GPIO_Init`只等待100ms，I2C引脚交给CubeMX；`OLED_SetCursor`先加列偏移，再发页号与列高/低位。图形库的 `OLED_Update`逐页发送8×128字节。时钟设置函数的增加见[模块15](15-oled-clock.md)，该函数须在接入当前完整 `OLED_Init`前完成。

### 第4步：只画一行字验收

在外设初始化后暂时调用，正式整合时删除此临时代码，由 `App_Init`初始化OLED：

```c
OLED_Init();
OLED_Clear();
OLED_ShowString(0, 0, "Hello World!", OLED_6X8);
OLED_Update();
```

## 验收与边界

先看英文，再画四角，核对列偏移和全屏范围。`OLED_HAL_LastStatus`只保存最后一次传输状态，不能证明整个初始化序列都成功。局部更新的X/宽度必须保证不越过显存行；游戏模块会在调用前专门裁剪。
