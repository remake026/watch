# 07 设置页与返回

[上一章](06-home.md) · [返回目录](../README.md) · [下一章：字段选择](08-fields.md)

## 本章目标

实现：首页确认设置 → 进入设置页 → 选择返回 → 回到首页。

原作者的设置页把“返回”和“日期时间设置”当作两个可选项。这里使用现有 ASCII 字库显示 `< Back` 和 `Date / Time`。

## 1. 设置页拥有自己的选中项

在全局用户变量区加入：

```c
uint8_t setFlag = 1;
```

| 变量 | 值1 | 值2 |
|---|---|---|
| `cliFlag` | 菜单 | 设置 |
| `setFlag` | 返回 | 日期时间设置 |

不要用同一个变量同时记录两个页面的选择。

## 2. 绘制设置页

在 `main()` 前的用户函数区加入：

```c
static void Show_SettingPage_UI(void)
{
    OLED_Clear();

    OLED_ShowString(0, 0, "< Back", OLED_8X16);
    OLED_ShowString(0, 24, "Date / Time", OLED_8X16);

    if (setFlag == 1)
    {
        OLED_ReverseArea(0, 0, 48, 16);
    }
    else
    {
        OLED_ReverseArea(0, 24, 88, 16);
    }

    OLED_Update();
}
```

`< Back` 有6个字符，占48像素；`Date / Time` 有11个字符，占88像素。

## 3. 设置页循环

放在绘图函数之后：

```c
static void SettingPage(void)
{
    setFlag = 1;

    while (1)
    {
        uint8_t key = Key_Event();

        if (key == 1)
        {
            if (setFlag > 1) setFlag--;
            else setFlag = 2;
        }
        else if (key == 2)
        {
            if (setFlag < 2) setFlag++;
            else setFlag = 1;
        }
        else if (key == 3)
        {
            if (setFlag == 1)
            {
                return;
            }
            else
            {
                /* 下一章在这里调用 SetTimePage(); */
            }
        }

        Show_SettingPage_UI();
    }
}
```

`return` 结束整个 `SettingPage()`，包括内部的循环，执行位置回到调用者。它不会直接跳转到某个名叫“首页”的函数。

## 4. 由主循环连接页面

删除上一章初始化区域中的 `Entry:` 测试，保留 OLED 初始化和 TIM2 启动。

用下面内容替换原来主循环中的按键数字测试：

```c
while (1)
{
    uint8_t entry = First_Page_Clock();

    if (entry == 1)
    {
        /* 后续接入菜单应用。 */
    }
    else if (entry == 2)
    {
        SettingPage();
    }
}
```

```mermaid
flowchart LR
    A[主循环调用首页] --> B[首页返回入口2]
    B --> C[调用设置页]
    C --> D[选择Back并确认]
    D --> E[设置页return]
    E --> A
```

菜单入口目前没有应用，确认后会进入主循环下一轮并再次显示首页。`Date / Time` 在本章仍是占位入口。

## 验证

1. 首页选择设置，键3进入。
2. 键1、键2在两项之间切换。
3. 选中 `< Back`，键3返回首页。
4. 重复进入和返回，首页不残留设置页文字。
