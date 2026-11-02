# 04 中文与字号

[上一章](03-oled.md) · [返回目录](../README.md) · [下一章：按键](05-keys.md)

## 本章目标

使用 `OLED_ShowString(..., OLED_12X24)` 显示“菜单”和“设置”，并理解 ASCII 字号与中文大小的关系。

## 1. 明确一个汉字占多大

本库中字号参数表示 ASCII 字符的宽度。同一个字号下，中文通常占两个 ASCII 字符的宽度。

| 字号 | ASCII字符 | 本教程中文 |
|---|---|---|
| `OLED_8X16` | 8×16 | 16×16 |
| `OLED_12X24` | 12×24 | 24×24 |

因此 `"菜单"` 在 `OLED_12X24` 下总宽48像素。128像素屏幕上：

- 两个字居中：`X=(128-48)/2=40`。
- 两个字右对齐：`X=128-48=80`。

文字从 `X=82` 开始会有两列落在画布之外。

## 2. 统一字符编码

将主程序、字库源文件和字库头文件**按原编码正确打开，再另存为 UTF-8 with BOM**。不要把原 GB2312 文件直接当 UTF-8 保存，否则可能保存已经乱码的内容。

在 `OLED_Data.h` 中只启用一个分支：

```c
#define OLED_CHARSET_UTF8
/* #define OLED_CHARSET_GB2312 */
```

UTF-8 下一个常用汉字通常占3字节，但显示宽度仍由字模决定。字节数不是屏幕像素宽度。

## 3. 增加24×24字模类型

在 `OLED_Data.h` 中加入：

```c
typedef struct
{
    char Index[5];
    uint8_t Data[72];
} ChineseCell24_t;

extern const ChineseCell24_t OLED_CF24x24[];
```

此定义配合本章固定采用的 UTF-8 配置。`Index[5]` 能容纳最长4字节的 UTF-8 编码及字符串结束符。

24×24图像需要：

```text
24列 × 24行 / 8 = 72字节
```

与配套 OLED 库一致，数据按照“先页后列、纵向8点、最低位在上”排列：

```text
前24字节：Y=0~7
中24字节：Y=8~15
后24字节：Y=16~23
```

把[附录 B](appendix-b-glyphs.md)的 `OLED_CF24x24` 数组放入 `OLED_Data.c`，包含菜、单、设、置四个字及最后的缺字占位图。数组应只定义一次。

## 4. 给字符串函数增加中文分支

在 `OLED_ShowString()` 中，库已经把当前字符提取到 `SingleChar`。找到处理多字节字符的 `else`，在原有16×16中文查表之前加入：

```c
if (FontSize == OLED_12X24)
{
    for (pIndex = 0;
         OLED_CF24x24[pIndex].Index[0] != '\0';
         pIndex++)
    {
        if (strcmp(OLED_CF24x24[pIndex].Index, SingleChar) == 0)
        {
            break;
        }
    }

    OLED_ShowImage(X + XOffset, Y, 24, 24,
                   OLED_CF24x24[pIndex].Data);
    XOffset += 24;
    continue;
}
```

找到字符时绘制对应字模；找不到时，循环到空索引，绘制最后的占位图。

`continue` 使该字符处理完成后进入下一个字符，不再重复执行原有16×16分支。

原库还需要具备 `OLED_12X24` 的 ASCII 字模和 `OLED_ShowChar()` 分支，用于后面的大号数字时钟。若所用库版本不包含，请先从配套版本补齐 `OLED_F12x24` 声明、数据及对应显示分支。

## 5. 验证显示

在 OLED 初始化后临时测试：

```c
OLED_Clear();
OLED_ShowString(0, 40, "菜单", OLED_12X24);
OLED_ShowString(80, 40, "设置", OLED_12X24);
OLED_Update();
```

两个标签都应完整显示。后续首页直接复用这个布局。

## 常见错误

- 16×16表中已有这几个字，但24×24表没有：两个字号使用不同的字模数据。
- 添加了24×24表但没有修改 `OLED_ShowString()`：表永远不会被访问。
- 索引编码与字符串不同：显示缺字占位图或乱码。
- 忘记 `XOffset += 24`：连续中文重叠。
- 缺少最后的空索引：查不到字符时可能越界读取。

后续设置页使用英文标签，以便先完成交互，再按需扩充其他汉字。
