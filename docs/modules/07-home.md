# 07 首页：显示时间并返回入口

[模块目录](README.md) · [上一章](06-app.md) · [下一章](08-settings.md)

## 设计思想

绘图函数只按状态画界面，页面循环负责读RTC和处理按键。首页返回入口编号，不在绘图函数里调用设置或菜单。这样应用层能清楚控制页面关系。

## 变量与逻辑

| 名称 | 作用 | 当前规则 |
|---|---|---|
| `static cliFlag=1` | 当前入口 | 1菜单、2设置；跨进入保留 |
| `time[6]` | 应用传来的共享缓存 | 每轮尝试RTC更新 |
| `key` | 本轮唯一按键事件 | 1上一项、2下一项、3确认 |
| `FIX_YEAR=2000` | 年份显示基数 | 只影响显示，不改缓存 |

键1让1→2、2→1，键2也在两项间循环；键3立即返回当前编号。一次循环顺序是“RTC读取 → 消费按键 → 更新选择/返回 → 重画”。

## A 移植：只复用显示能力

1. 接入已验证的RTC、按键和中文显示接口；不复制原首页中的标准库GPIO操作。
2. 在 `home_page.h`声明 `uint8_t HomePage_Run(uint8_t time[6]);`，Keil注册 `home_page.c`。
3. 原页面布局可以参考，时间结构改为本工程的六字段数组。当前文件对照 [home_page.c](reference/bsp/home_page.c)。

## B 自己编码：先画静态页，再加选择与循环

### 第1步：写静态绘图函数

在 `.c`包含 `home_page.h/main.h/rtc.h/OLED.h/key.h`，定义 `FIX_YEAR`与 `cliFlag`，再写 `static void Show_Ui(const uint8_t time[6])`。依次清屏、绘年月日、绘时分秒、绘两个中文入口：

```c
OLED_Clear();
OLED_Printf(0, 0, OLED_8X16, "%02d-%02d-%02d",
            2000 + time[0], time[1], time[2]);
OLED_Printf(16, 16, OLED_12X24, "%02d:%02d:%02d",
            time[3], time[4], time[5]);
OLED_ShowString(0, 40, "菜单", OLED_12X24);
OLED_ShowString(80, 40, "设置", OLED_12X24);
```

`%02d`是最小宽度2，不会把2026截成26。先用固定缓存观察排版。

### 第2步：按选择反色

在同一绘图函数末尾：

```c
if (cliFlag == 1) OLED_ReverseArea(0, 40, 48, 24);
else OLED_ReverseArea(80, 40, 48, 24);
OLED_Update();
```

每轮清屏重画后只反色一次，避免在已有反色显存上不断翻转导致闪烁。

### 第3步：替换首页占位函数

```c
uint8_t HomePage_Run(uint8_t time[6])
{
    while (1) {
        uint8_t key;
        RTC_ReadDateTime(time);
        key = Key_Event();
        if (key == 1) cliFlag = (cliFlag == 1) ? 2 : 1;
        else if (key == 2) cliFlag = (cliFlag == 2) ? 1 : 2;
        else if (key == 3) return cliFlag;
        Show_Ui(time);
    }
}
```

以上用条件表达式写出与当前源码增减回绕等价的逻辑，完整快照保留原写法。`cliFlag`只在初始化时设1，进入首页时不重置，因此返回后保留选择。

## 验收与边界

键1/2都能切换，键3返回后 `App_Run`进入对应页；秒显示随RTC变化。设置只改RAM时，回首页会被RTC重新读取覆盖，这是当前未写回RTC的结果。当前首页每轮全屏刷新，优化成秒变化或按键时刷新属于后续练习。
