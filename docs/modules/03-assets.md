# 03 字库、图标与位图

[模块目录](README.md) · [上一章](02-oled.md) · [下一章](04-keys.md)

## 设计思想

图像数据与页面逻辑分开。`OLED_Data.c`定义常量数组，`OLED_Data.h`公开尺寸与声明；页面决定画什么、画在哪，不在页面函数里粘贴几百行字节。先证明单张图片格式正确，再做动画或碰撞。

## 变量与逻辑

| 数据 | 格式/占用 | 使用者 |
|---|---|---|
| `OLED_CF16x16` | 每个字32字节+索引 | 秒表等中文 |
| `OLED_CF24x24` | 每个字72字节+索引 | 首页“菜单/设置” |
| `Menu_Graph[7][128]` | 7张32×32图标 | 菜单 |
| `ExitArrow` | 24×24，72字节 | 秒表返回 |
| `Panda_Run[6][128]` | 6张32×32，768字节 | 游戏和演示 |
| `Cactus[48]` | 16×24，48字节 | 游戏 |
| `Kun_DancePacked/Offsets` | 压缩帧与46个偏移 | 动画解码 |

纵向8点格式：`index = (y / 8) * width + x`，位掩码 `1U << (y % 8)`；每张图字节数 `width * ((height + 7) / 8)`。例如 `(x=3,y=9)`位于第二页第3列、bit1，不是第9行第3个字节。

## A 移植：整理数据而非重写素材

1. 保留原ASCII和中文库；UTF-8与GB2312只启用一种，当前启用 `OLED_CHARSET_UTF8`。
2. 从原课程复制菜单图标和返回箭头，声明尺寸与定义一致；原 `Frame`数组虽保留，当前菜单边框由画线完成。
3. 熊猫/仙人掌按[素材说明](../oled-panda.md)核对六帧预览；KUN动画按[来源记录](../kun-dance.md)取得配套压缩数组与偏移表。
4. `Eyebrow/Mouth`是旧六帧表情用的数据，仍保留在数据文件；当前动画页不调用它们。
5. 保存含中文源码为UTF-8 BOM，索引字节必须与页面字符串一致。

## B 自己编码：从数据接口到绘制封装

### 第1步：先声明尺寸，再声明数组

```c
#define OLED_PANDA_WIDTH      32U
#define OLED_PANDA_HEIGHT     32U
#define OLED_PANDA_RUN_FRAMES  6U
#define OLED_PANDA_RUN_BYTES 128U
extern const uint8_t Panda_Run[OLED_PANDA_RUN_FRAMES][OLED_PANDA_RUN_BYTES];
```

在 `.h`只放 `extern`声明，在 `.c`放唯一的数组定义。当前全部尺寸对照 [OLED_Data.h](reference/bsp/OLED_Data.h)。

### 第2步：补24×24中文分支

先定义 `ChineseCell24_t`的索引和 `Data[72]`，再加入“菜、单、设、置”及缺字占位；`OLED_ShowString`根据字号查对应字库，找到后调用 `OLED_ShowImage(X,Y,24,24,Data)`，中文光标前进24像素。原尺寸名 `OLED_12X24`表示ASCII宽度，不意味着中文也只有12像素。

字模和字符串处理的完整练习见[中文与字号](../04-fonts.md)及[24×24数据附录](../appendix-b-glyphs.md)。先验证一个中文，再拼“菜单”。

### 第3步：给游戏写小封装

在 `OLED.c`增加唯一实现，在 `OLED.h`声明：

```c
void OLED_ShowPandaRun(int16_t X, int16_t Y, uint8_t FrameIndex)
{
    OLED_ShowImage(X, Y, OLED_PANDA_WIDTH, OLED_PANDA_HEIGHT,
                   Panda_Run[FrameIndex % OLED_PANDA_RUN_FRAMES]);
}
void OLED_ShowCactus(int16_t X, int16_t Y)
{
    OLED_ShowImage(X, Y, OLED_CACTUS_WIDTH, OLED_CACTUS_HEIGHT, Cactus);
}
```

### 第4步：先显示单帧

清屏后分别显示第0帧、第5帧和仙人掌，最后 `OLED_Update`。动画定时放在页面，不放在素材显示封装中。当前 `OLED_ShowImage`会清除目标矩形的旧内容，两个重叠精灵直接依次调用可能擦掉前一个，游戏章单独处理。

## 验收与边界

检查图像不是横竖颠倒、页顺序正确、字库无乱码；确认 `.h`的尺寸与 `.c`实际数据一致。位图封装对照 [OLED.h](reference/bsp/OLED.h)，45帧的解码逻辑留到[模块14](14-animation.md)。
