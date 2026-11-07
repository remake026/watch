# 05 RTC读取与时间数据

[模块目录](README.md) · [上一章](04-keys.md) · [下一章](06-app.md)

## 设计思想

RTC负责走时，`my_time`只是给页面显示/编辑的RAM缓存。读取函数负责把HAL结构转换成6字节；设置页面改数组，不等于写入RTC。先把“读”做通，再把写回、日期校验和掉电保存作为独立功能设计。

## 变量与逻辑

| 变量 | 类型/作用 |
|---|---|
| `before` | 首次 `RTC_TimeTypeDef`，用于跨小时检查 |
| `date` | `RTC_DateTypeDef`，年月日 |
| `now` | 第二次时间读数，提供时分秒 |
| `value[6]` | 调用者提供的输出缓存，年份为2000偏移 |
| 返回值 | 1成功；0失败，保持调用者数组原值 |

当前顺序是“读时间 → 读日期 → 再读时间 → 比较小时 → 一次性写六字段”。如果跨小时则返回0，下一轮首页再试；它是当前实现的有限检查，不是完整的RTC故障处理。

## A 移植：保留HAL RTC，核对器件差异

1. 用CubeMX启用LSE与RTC，保留 `hrtc`和HAL驱动；新接口写在 `rtc.c/.h`用户区域。
2. 初始化使用 `RTC_FORMAT_BCD`时，`0x21`表示十进制21；读取输出用 `RTC_FORMAT_BIN`，不要再当BCD转换一次。
3. 原标准库的日期数组若保存完整年份，适配时要转换为当前年份偏移，不直接塞进 `uint8_t`。
4. STM32F1 HAL的日期由软件维护，不能照搬其他STM32系列的日历持久化假设。对照[ST官方RTC驱动说明](https://github.com/STMicroelectronics/stm32f1xx-hal-driver/blob/master/Src/stm32f1xx_hal_rtc.c)及本地 `Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rtc.c`开头说明。

## B 自己编码：成功前不修改输出

### 第1步：声明输出接口

在 `Core/Inc/rtc.h`的用户声明区加入：

```c
uint8_t RTC_ReadDateTime(uint8_t value[6]);
```

### 第2步：先读到局部变量

在 `rtc.c`的 `USER CODE BEGIN 1`写：

```c
uint8_t RTC_ReadDateTime(uint8_t value[6])
{
    RTC_TimeTypeDef before = {0};
    RTC_TimeTypeDef now = {0};
    RTC_DateTypeDef date = {0};
    if (HAL_RTC_GetTime(&hrtc, &before, RTC_FORMAT_BIN) != HAL_OK) return 0;
    if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) return 0;
    if (HAL_RTC_GetTime(&hrtc, &now, RTC_FORMAT_BIN) != HAL_OK) return 0;
    if (before.Hours != now.Hours) return 0;

    value[0] = date.Year;
    value[1] = date.Month;
    value[2] = date.Date;
    value[3] = now.Hours;
    value[4] = now.Minutes;
    value[5] = now.Seconds;
    return 1;
}
```

这样不会读到月份后遇到错误，却留下半新半旧的输出。当前函数要求非空6字节缓冲，无空指针检查。完整当前初始化见 [rtc.c快照](reference/Core/Src/rtc.c)。

### 第3步：显示与验证

在临时测试循环中先检查返回值，成功时用 `2000 + value[0]`显示年份。正式首页当前每轮调用并忽略返回值，失败时继续显示上次值；失败提示可作为后续改进。

## 验收与边界

连续观察秒递增和跨分钟，分别记录复位后的表现。当前 `MX_RTC_Init`每次仍设置默认时分秒与日期，且没有备份域初始化标记，不能宣称“重启保留”。只接VBAT也不能修复应用每次覆盖初始化的问题。

后续改进练习：先设计合法日期判定，再加写回接口并处理失败；最后设计备份寄存器标记、日期保存/恢复和跨日验证。以上尚未接入当前固件，本章不把它们写成已完成代码。
