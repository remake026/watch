# 14 动画：素材移植、解码算法、页面播放分别写

[模块目录](README.md) · [上一章](13-game-page.md) · [下一章](15-oled-clock.md)

## 设计思想

动画分三层：只读压缩字节保存素材；解码函数返回一张普通位图；页面决定何时换帧和退出。这样换素材时不必重写按键，调整播放速度也不用重写压缩算法。

## 变量与逻辑

| 变量 | 作用/大小 |
|---|---|
| `Kun_DancePacked` | 当前45帧的压缩数据7326字节 |
| `Kun_DanceOffsets[46]` | 每帧起点与末尾哨兵，92字节 |
| `Kun_DanceBuffer[504]` | 当前解码帧，72×56/8 |
| `Kun_DanceLastFrame` | 缓冲对应的帧号 |
| `Kun_DanceBufferValid` | 是否已有可复用帧 |
| 页面 `frame` | 当前播放帧0～44，进入归0 |
| `lastFrame/now` | 换帧基准，目标间隔60ms |

相邻帧先异或得到差分，零差分段表示不变；非零段携带需要异或回缓冲的字节。第0帧以全零缓冲为基准。只有顺序请求下一帧才能直接应用一个差分，跳转和回到0必须重建。

## A 移植：先取得可核对的素材

1. 来源、裁切与预览见[KUN素材记录](../kun-dance.md)，保留来源说明。
2. 从当前配套 `OLED_Data.c`取得成对的压缩数组与偏移表，在 `OLED_Data.h`声明72×56、45帧、504字节、60ms及解码接口。两张表必须来自同一次编码。
3. 这一步移植的是数据；不要让学习者手抄7326个字节。当前教程快照不重复打包整份字库/数据文件。
4. 早期 `Eyebrow/Mouth`六帧表情仍有[历史教程](../12-emoji.md)，当前菜单5已经替换为KUN，不能把旧入口说明当成当前实现。

## B 自己编码：先能解一帧，再循环播放

### 第1步：理解token，再写差分应用

在 `OLED_Data.c`数组之后定义三个静态缓存变量。`Kun_ApplyFrameDelta`按 `Offsets[frame]`和 `Offsets[frame+1]`限定输入区间，输出位置从0开始：

```c
uint8_t token = *source++;
uint16_t length = (uint16_t)(token & 0x7FU) + 1U;
if (token & 0x80U) {
    for (uint16_t i = 0; i < length; ++i)
        Kun_DanceBuffer[position++] ^= *source++;
} else {
    position += length;
}
```

上段放在 `while(source<end)`中；高位0表示跳过1～128字节，高位1表示跟随1～128个字节并逐个异或。例：全零缓存遇到 `{0x01,0x81,0x05,0x80}`，先跳过2字节，再将位置2/3异或为05/80。token长度包含加1，漏掉会错位。

当前解码器信任固件内置合法数据，没有为任意外部文件做完整边界校验；若以后接收可替换文件，需另加输出≤504、输入剩余长度及偏移单调性验证。

### 第2步：写帧获取状态机

`OLED_GetKunDanceFrame`按以下顺序实现：

```text
帧号先对45取模
若缓冲有效且请求等于上次：直接返回缓存
否则若等于上次+1：只应用该帧差分
否则：清504字节缓存，从第0帧依次应用到目标帧
记录最后帧号，设有效，返回缓存地址
```

这一步自己实现的是解码逻辑，不重新绘制素材。返回指针指向共享缓存，画完当前帧再请求另一帧，不能保存两个指针当成两张独立图片。

### 第3步：写页面首次画面

新建 `animation_page.h/.c`，只公开 `AnimationPage_Run`。进入时 `frame=0`，清屏、写顶行 `3:OUT`和 `KUN DANCE`，然后：

```c
OLED_ShowImage(28, 8, OLED_KUN_DANCE_WIDTH, OLED_KUN_DANCE_HEIGHT,
               OLED_GetKunDanceFrame(frame));
OLED_Update();
lastFrame = HAL_GetTick();
```

### 第4步：写换帧与返回

```c
if (Key_Event() == 3) return;
now = HAL_GetTick();
if ((uint32_t)(now - lastFrame) >= OLED_KUN_DANCE_FRAME_MS) {
    lastFrame = now;
    if (++frame >= OLED_KUN_DANCE_FRAMES) frame = 0;
    OLED_ShowImage(28, 8, OLED_KUN_DANCE_WIDTH, OLED_KUN_DANCE_HEIGHT,
                   OLED_GetKunDanceFrame(frame));
    OLED_UpdateArea(28, 8, OLED_KUN_DANCE_WIDTH, OLED_KUN_DANCE_HEIGHT);
}
HAL_Delay(1);
```

放在页面永久循环中，键1/2被消费但不产生动作。等待期间每轮仍检查退出，不能用60ms阻塞延时直接替代调度。当前超时只推进一帧，不补算落下的帧，实际速度仍受通信影响。

完整播放页对照 [animation_page.c](reference/bsp/animation_page.c)，菜单5调用后仍由菜单恢复画面。

## 验收与边界

先核对第0帧，再测连续45帧回到0、重复请求同帧、直接请求第20帧、离开后重进。整轮目标2.7秒，观察动作顺序与[GIF](../assets/kun-dance.gif)一致；顶行固定文字不跟着闪动，键3能退出。压缩节省Flash，解码缓存仍占504字节RAM，不能把两者混为一谈。
