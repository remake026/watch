# 08 设置：字段选择与编辑分成两个状态

[模块目录](README.md) · [上一章](07-home.md) · [下一章](09-menu.md)

## 设计思想

“选中年份”与“正在改年份”不是同一状态。浏览状态键1/2移动字段，键3进入编辑；编辑状态键1/2改数值，键3退回浏览。用同一绘图函数，根据 `editing`决定反色整行还是仅数字。

## 变量与逻辑

| 名称 | 作用 |
|---|---|
| 文件级 `setFlag` | 当前选择1～7，进入设置页重置1 |
| `SetTime`形参 `setFlag` | 本次编辑字段的副本，遮蔽同名全局但不修改它 |
| `time[setFlag-2]` | 编号2～7映射数组0～5 |
| `row` | 当前屏幕行号0～3 |
| `editing` | 0浏览反色整行，1编辑反色数字 |
| `numberX/numberWidth` | 数字反色区域，容纳当前2或3位值 |

| setFlag | 内容 | 数组下标 | 屏幕/行 |
|---|---|---|---|
| 1 | 返回 | 无 | 第一屏/0 |
| 2 | 年 | 0 | 第一屏/1 |
| 3 | 月 | 1 | 第一屏/2 |
| 4 | 日 | 2 | 第一屏/3 |
| 5 | 时 | 3 | 第二屏/0 |
| 6 | 分 | 4 | 第二屏/1 |
| 7 | 秒 | 5 | 第二屏/2 |

## A 移植：接入显示与交互约定

1. 创建 `settings_page.h/.c`，声明 `void SettingsPage_Run(uint8_t time[6]);`，替换应用章的设置占位函数。
2. 参考原设置交互，但使用当前英文标签与数组；不搬进另一套 `MyRTC_Time`全局变量。
3. 当前完整对照：[settings_page.c](reference/bsp/settings_page.c)。先复现当前行为，再独立改范围校验，不把两种版本混在同一个实现里。

## B 自己编码：先浏览，再编辑

### 第1步：先画两屏

写 `Show_SettingPage_UI(time,editing)`：`setFlag<=4`时画Back/year/mon/day，`row=setFlag-1`；否则画hour/min/sec，`row=setFlag-5`。行高16，Y为 `row*16`，最后只刷新一次。

### 第2步：加入反色分支

```c
if (editing && setFlag >= 2) {
    uint8_t numberX = (setFlag == 2 || setFlag == 5) ? 48 : 40;
    uint8_t numberWidth = (time[setFlag - 2] >= 100) ? 24 : 16;
    OLED_ReverseArea(numberX, row * 16, numberWidth, 16);
} else {
    OLED_ReverseArea(0, row * 16, 128, 16);
}
```

标签长度不同，所以year/hour数字从48开始，其余从40开始。先手动设 `setFlag=4/5`检查翻屏边界。

### 第3步：写浏览状态循环

```c
void SettingsPage_Run(uint8_t time[6])
{
    setFlag = 1;
    while (1) {
        uint8_t key = Key_Event();
        if (key == 1) setFlag = (setFlag > 1) ? setFlag - 1 : 7;
        else if (key == 2) setFlag = (setFlag < 7) ? setFlag + 1 : 1;
        else if (key == 3) {
            if (setFlag == 1) return;
            SetTime(time, setFlag);
        }
        Show_SettingPage_UI(time, 0);
    }
}
```

先在此函数前声明/定义 `static void SetTime(uint8_t time[6],uint8_t setFlag)`，初次用空体验收浏览，下一步替换。

### 第4步：写编辑循环

当前实现可拆为以下相同顺序，在 `SetTime`的永久循环内执行：

```c
uint8_t key = Key_Event();
if (key == 3) return;
if (setFlag <= 4) { /* 年/月/日：当前直接改uint8_t */
    if (key == 1) time[setFlag - 2]--;
    else if (key == 2) time[setFlag - 2]++;
} else { /* 时/分/秒：当前共用不完整的0~59处理 */
    if (key == 1) time[setFlag - 2]--;
    else if (key == 2) {
        time[setFlag - 2]++;
        if (time[setFlag - 2] > 59) time[setFlag - 2] = 0;
    }
}
Show_SettingPage_UI(time, 1);
```

这是当前有效行为的拆解。原源码还有 `uint8_t < 0`判断，它永远为假，不提供下限保护；本示意省略无效语句，未修改固件文件。

## 验收与边界

先用正常数值测试六字段各加/减一次，再验证键3只退到浏览，Back才退回首页。当前行为有明确缺陷：年/月/日可溢出，时错误地允许到59，0减1会变255，没有闰年/月天数判断，也没有RTC写回。回首页后RTC读取会覆盖编辑缓存。

后续改进练习单独设计：用有符号临时量运算 → 按字段限定年0～99、月1～12、时0～23、分秒0～59 → 按年月限定日 → 确认写入RTC并处理错误。不要把这些规则描述为当前已实现。
