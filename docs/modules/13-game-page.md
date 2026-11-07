# 13 游戏页面：输入、固定步长与局部刷新

[模块目录](README.md) · [上一章](12-game-logic.md) · [下一章](14-animation.md)

## 设计思想

规则每40ms更新，画面只把变化区域发到OLED。一次较慢的显示传输可能跨过几个逻辑步，先补算规则，再画最终状态。清除旧精灵、重画新精灵和发送显存是三个不同动作，漏掉任何一个都会残影或丢图。

## 变量与逻辑

| 变量/函数 | 功能 |
|---|---|
| `GameState game` | 本次进入的一局，进入时复位 |
| `GameState old` | 本轮按键/运动前的状态，供差分绘图 |
| `static uint16_t best` | 本次上电最高分，跨页面保留 |
| `lastStep/now` | 规则基准与当前毫秒时间 |
| `steps` | 已过去的步数，单轮最多补4步 |
| `GameUpdateArea` | 用有符号坐标裁剪矩形后再发屏 |
| `GameDrawScene/Changes` | 全屏状态切换/运行期差分刷新 |

键1跳跃，OVER时重开；键2运行/暂停切换；键3返回。暂停恢复或重开时重设时间基准，不补算暂停期间。进入游戏总是新一局，最高分不清零。

## A 移植：复用规则与驱动，先全屏跑通

1. 注册 `game.c`与 `game_page.c`，头文件声明 `void GamePage_Run(void);`。
2. 接入已验收的 `Game_Reset/Step/Jump/TogglePause`与素材接口。
3. 先使用全屏 `GameDrawScene`验证，再改运行期局部刷新，避免同时排查碰撞和显存问题。
4. 完整当前实现：[game_page.c](reference/bsp/game_page.c)。

## B 自己编码：分五次增加能力

### 第1步：先写完整场景

`GameDrawScene`依次清屏→角色和障碍→Y62地面线→顶部得分→按状态显示暂停/结束文字→全屏发送。`GameDrawScore`先清顶行再画 `S:%04u HI:%04u`。

`GameDrawSprites`先调用熊猫绘制；仙人掌逐个亮像素使用 `OLED_DrawPoint`叠加。原因是当前 `OLED_ShowImage`会清目标矩形，直接画仙人掌矩形可能擦掉与其重叠的熊猫透明/可见区域。叠加位读取为：

```c
if (Cactus[(y / 8U) * OLED_CACTUS_WIDTH + x] & (1U << (y % 8U)))
    OLED_DrawPoint(game->cactusX + x, GAME_CACTUS_Y + y);
```

### 第2步：写输入与状态切换

进入时复位、全屏画一次、保存tick。每轮先保存 `old=game`再处理事件。键3直接返回；键1结束时复位重画并 `continue`，否则起跳；键2状态确实改变时全屏重画、重设tick并 `continue`。先只看暂停和重开是否正确。

### 第3步：加入固定步长

```c
if (game.status != GAME_RUNNING || (uint32_t)(now - lastStep) < GAME_STEP_MS) {
    HAL_Delay(1);
    continue;
}
steps = (uint32_t)(now - lastStep) / GAME_STEP_MS;
lastStep += steps * GAME_STEP_MS;
if (steps > 4U) steps = 4U;
while (steps-- > 0U && game.status == GAME_RUNNING) Game_Step(&game);
```

先按全部已过去步数推进时间基准，再限制实际补算4步，因此过久阻塞时主动丢弃更早的积压，不会无限追赶。这个取舍保持输入可响应，但不保证长阻塞期间完全实时模拟。每个补算步都做碰撞，不能只算最后一帧。

### 第4步：写安全的发送矩形

```c
static void GameUpdateArea(int16_t x, int16_t y, int16_t width, int16_t height)
{
    int16_t right = x + width, bottom = y + height;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right > 128) right = 128;
    if (bottom > 64) bottom = 64;
    if (right > x && bottom > y)
        OLED_UpdateArea(x, y, (uint8_t)(right - x), (uint8_t)(bottom - y));
}
```

必须先在有符号类型内裁剪，再转换宽高；障碍会离开屏幕，直接转 `uint8_t`可能变成很大的值。OLED驱动按页发数据，逻辑矩形可能扩大到页边界。

### 第5步：实现差分绘制

按快照 `GameDrawChanges`的顺序逐条写：

1. 清旧熊猫矩形和旧障碍矩形，再把所有新精灵画回RAM。
2. 熊猫Y或动画帧变化时，发送旧/新熊猫矩形的纵向并集。
3. 障碍正常向左时发送旧/新位置并集；重生跳回右侧时，分别发旧区和新区，避免整屏宽发送。
4. 分数变化或旧/新熊猫顶部进入Y<8时，重画并发送顶部得分。位图透明行也会清显存，跳跃顶端可能碰到HUD区域。
5. 结束状态全屏重画，正常运行用差分；更新 `best`后再绘制。

## 验收与边界

先测试跳跃、暂停恢复、死亡重开、返回重进，再观察障碍左边界/重生、熊猫跳跃最高点和得分文字是否残缺。现有渲染测试把局部更新后的模拟屏幕与完整场景对照，能发现漏发区域；真实总线速度、屏幕闪烁仍需板上确认。最高分没有Flash持久化。
