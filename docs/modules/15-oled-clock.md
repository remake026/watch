# 15 OLED时钟测试：驱动设置与测试页面分开

[模块目录](README.md) · [上一章](14-animation.md) · [整合验收](16-integration.md)

## 设计思想

驱动负责发送参数并报告成功与否，页面负责选择档位、显示结果。画面保持静态，避免把频繁写屏引入测试；拍摄闪烁是否改善由实物对比决定，不能把最高档当作已经验证的最佳值。

## 变量与逻辑

| 名称 | 功能 |
|---|---|
| `OLED_DisplayClock` | 最近完整发送成功的参数缓存，静态初值0x80 |
| `clock` | D5参数，高4位振荡器档位，低4位分频减1 |
| `oscillator` | `clock >> 4`，范围0～15 |
| `applied` | 本次设置函数返回1成功/0失败 |
| `failed` | 绘图函数参数，是否显示I2C ERROR |

键1档位减1，0回绕15；键2加1，15回绕0；低4位始终0，即分频1；键3返回并保留本次上电参数。`O/D`是档位与分频值，不是测得的Hz。

## A 移植：只使用已核对的命令和总线

1. 使用现有OLED写命令接口；D5字段参考[静态测试说明及原厂数据手册链接](../oled-camera-flicker.md)。不在测试页重配I2C总线。
2. 新建 `oled_test_page.h/.c`，公开 `void OLEDTestPage_Run(void);`并注册Keil。
3. 当前初始化宏为 `WATCH_OLED_DISPLAY_CLOCK=0xF0`，是候选值；旧值0x80可用于对照。没有自动识别屏幕控制器。

## B 自己编码：先补驱动接口，再写页面

### 第1步：给OLED驱动增加可返回状态的设置

在 `OLED.c`定义静态缓存，在 `OLED.h`加入两个接口声明：

```c
uint8_t OLED_SetDisplayClock(uint8_t Clock);
uint8_t OLED_GetDisplayClock(void);
```

函数体按当前源码：

```c
uint8_t OLED_SetDisplayClock(uint8_t Clock)
{
    OLED_WriteCommand(0xD5);
    if (OLED_HAL_LastStatus != HAL_OK) return 0;
    OLED_WriteCommand(Clock);
    if (OLED_HAL_LastStatus != HAL_OK) return 0;
    OLED_DisplayClock = Clock;
    return 1;
}
uint8_t OLED_GetDisplayClock(void)
{
    return OLED_DisplayClock;
}
```

两个字节都成功才更新软件缓存，失败保留上次缓存。它记录的是最后完整成功请求，不是读取硬件寄存器的测量值。

### 第2步：接入启动参数

在 `OLED_Init`原D5设置处替换为 `(void)OLED_SetDisplayClock(WATCH_OLED_DISPLAY_CLOCK);`。当前启动处没有利用返回值做错误恢复，这是源码现状。

### 第3步：写静态测试画面

`Show_OLEDClockTest(failed)`绘制标题、D5十六进制数、`O:%02u D:%02u`、中文与固定横线；失败时写 `I2C ERROR`；底部写 `1:-  2:+  3:USE`，最后全屏发送。完整坐标对照 [oled_test_page.c](reference/bsp/oled_test_page.c)。

### 第4步：写档位循环

```c
uint8_t oscillator = OLED_GetDisplayClock() >> 4;
if (key == 1) oscillator = (oscillator == 0U) ? 15U : oscillator - 1U;
else oscillator = (oscillator + 1U) & 0x0FU;
uint8_t clock = (uint8_t)(oscillator << 4);
uint8_t applied = OLED_SetDisplayClock(clock);
Show_OLEDClockTest((uint8_t)!applied);
```

上段只放在 `key==1 || key==2`分支；页面进入画一次，键3直接返回，其余循环只 `HAL_Delay(1)`，没有按键时不重复刷新显存。

### 第5步：接菜单6并记录实测

固定手机距离、曝光及环境，对比全部档位，每次等待画面稳定，记下最好参数，再修改启动宏重新烧录。当前没有Flash保存，重新上电回到宏值。

## 验收与边界

测试0/F回绕、成功后缓存改变、失败提示、返回后其他页面继续使用参数。当前主机页面测试只模拟成功，不覆盖真实I2C故障；需实机验证失败路径及显示效果。命令应答不能证明芯片型号、实际扫描频率或拍摄问题已解决。
