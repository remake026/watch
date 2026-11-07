# STM32F103C8T6 手表完整工程

[仓库首页](../README.md) · [模块化教程](../docs/modules/README.md)

此目录包含当前本地固件工程源码、库依赖、CubeMX配置、Keil项目与主机测试。按教程手写时，可以逐个模块与 `bsp`对照；直接构建时使用此目录下的工程。

## 打开和构建

1. 用Keil µVision打开 [MDK-ARM/watch_1-2.uvprojx](MDK-ARM/watch_1-2.uvprojx)。当前工程配置为Arm Compiler 5.06 update 7，设备包 `Keil.STM32F1xx_DFP.2.3.0`；安装相应工具链/设备包后再构建。
2. 目标为STM32F103C8，HSI 8MHz。源码所需HAL/CMSIS已随目录提供，头文件与源文件路径均相对项目目录。
3. 执行Rebuild。固件输出在 `MDK-ARM/watch_1-2/`，由Git忽略，不提交本机生成的二进制与中间文件。
4. 烧录时按自己的探针配置SWD与下载选项；仓库不包含个人调试器配置。
5. 如需修改外设，用CubeMX打开 [watch_1-2.ioc](watch_1-2.ioc)，生成前保留用户代码，生成后核对 `bsp`源文件注册与工程编译配置。

## 硬件约定

| 功能 | 当前接线 |
|---|---|
| OLED I2C1 | PB8 SCL / PB9 SDA，重映射，100kHz，7位地址0x3C |
| 按键1/2/3 | PB1 / PA6 / PA4，上拉输入，按下接地 |
| 输出 | PB15，初始低电平 |
| 扫描时基 | TIM2每1ms中断，按键20ms间隔采样 |
| RTC | LSE 32.768kHz，需对应实物晶振 |
| 调试 | PA13/PA14，SWD |

## 测试

在本目录执行，需要PATH中有GCC与PowerShell：

```powershell
./tests/run_game_tests.ps1
./tests/run_page_tests.ps1
```

游戏测试检查规则和模拟显存，页面测试检查9组交互。它们不代替真实I2C、RTC、按键与屏幕的板上验收。

## 功能与已知边界

当前支持首页、设置、滑动菜单、PB15切换、秒表、熊猫游戏、KUN动画和OLED时钟测试。设置字段与合法日期校验、RTC编辑写回、日期重启保留仍待完善；秒表为当前秒级实现。详见[模块范围](../docs/modules/README.md)及[应用与页面说明](bsp/README.md)。

HAL、CMSIS保留各自目录的许可证；OLED保留江协科技原署名。原设计与课程、图像素材来源见[仓库来源说明](../README.md)和[KUN素材记录](../docs/kun-dance.md)。
