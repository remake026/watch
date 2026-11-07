# 12 游戏规则：先算状态，不操作屏幕

[模块目录](README.md) · [上一章](11-stopwatch.md) · [下一章](13-game-page.md)

## 设计思想

`game.c`回答“一步之后角色在哪、有没有撞、得几分”，不读取HAL时钟，不画OLED。页面每40ms让规则前进一步，即使绘图速度变化，运动单位仍然固定。碰撞需要位图，但不需要真实屏幕，因此能在电脑上测试。

## 变量与逻辑

| `GameState`字段 | 含义/单位 | 初值 |
|---|---|---|
| `pandaY16` | Y坐标，1/16像素，无符号16位 | `30*16` |
| `velocity16` | 每逻辑步Y增量，1/16像素，有符号 | 0；起跳设-60 |
| `cactusX` | 障碍物X，有符号像素 | 128，屏幕右外 |
| `score` | 已通过障碍数 | 0，上限9999 |
| `frame` | 熊猫帧0～5 | 0 |
| `animationTicks` | 距上次换帧的逻辑步数 | 0，每2步换帧 |
| `status` | RUNNING/PAUSED/OVER | RUNNING |

屏幕Y向下为正，所以负速度向上。40ms一步，每步速度加5产生重力；熊猫回到Y30时钳位并把速度归0。每80ms换一帧，障碍速度按分数为3/4/5像素每步。

## A 移植：只带入位图和尺寸契约

1. 从素材模块取得 `Panda_Run`与 `Cactus`，声明在 `OLED_Data.h`，不能拿一张预览PNG代替C数组。
2. 新建 `game.h/.c`，`game.h`只依赖 `stdint.h`；`game.c`额外依赖位图声明，不包含 `main.h`或OLED驱动接口。
3. 原小恐龙交互可参考，当前是熊猫素材、定点运动与逐位碰撞，按本章手写规则。
4. 完整对照：[game.h](reference/bsp/game.h)、[game.c](reference/bsp/game.c)。

## B 自己编码：状态→运动→碰撞→计分

### 第1步：先写状态结构和复位

按上表定义结构，再在 `Game_Reset`把每个字段明确赋初值。`Game_PandaY`返回 `pandaY16/16U`给绘图用。当前这些短函数放在头文件中并用 `static inline`，每个包含者使用自己的内部定义，不另加冲突的外部定义。

### 第2步：限制起跳与暂停

```c
static inline void Game_Jump(GameState *game)
{
    if (game->status == GAME_RUNNING &&
        game->pandaY16 == GAME_PANDA_REST_Y * 16 &&
        game->velocity16 == 0)
        game->velocity16 = -60;
}
```

只有落地且正在运行才起跳，禁止空中连跳。`Game_TogglePause`只在RUNNING和PAUSED间切换，OVER不因暂停键复活。先在电脑中调用复位、起跳，检查状态值，无需页面。

### 第3步：写一小步运动

在 `Game_Step`先判断非运行则直接返回，再按固定顺序执行：

```c
game->pandaY16 += game->velocity16;
game->velocity16 += 5;
if (game->pandaY16 >= GAME_PANDA_REST_Y * 16) {
    game->pandaY16 = GAME_PANDA_REST_Y * 16;
    game->velocity16 = 0;
}
game->cactusX -= Game_ObstacleSpeed(game);
if (++game->animationTicks >= OLED_PANDA_FRAME_MS / GAME_STEP_MS) {
    game->animationTicks = 0;
    game->frame = (uint8_t)((game->frame + 1U) % OLED_PANDA_RUN_FRAMES);
}
```

初始速度、重力与地面位置共同保证当前跳跃轨迹不越过Y0。若增强跳跃，必须检查无符号位置下溢，不能只改一个负速度常量。

### 第4步：从粗矩形理解到逐位碰撞

先在纸上画两个位图矩形，再把碰撞区域收紧到素材内部：熊猫X `[6,26)`、Y `[4,28)`；仙人掌X `[3,13)`、Y `[2,22)`，右/下边界不包含。

当前实现不是“矩形相交就死”，而是：

1. 得到 `pandaY`与垂直差 `shift=GAME_CACTUS_Y-pandaY`，若 `shift>=32`则上下不相交。
2. 遍历仙人掌有效列 `cx`，转换到熊猫列 `px=cactusX+cx-GAME_PANDA_X`，不在熊猫碰撞范围则跳过。
3. 把熊猫同列4个页字节拼成32位，把仙人掌同列3个页字节拼成24位。
4. 用位掩码去掉碰撞区外的顶部/底部，再对齐Y坐标做按位与。

```c
p &= (1UL << GAME_PANDA_HIT_BOTTOM) - (1UL << GAME_PANDA_HIT_TOP);
c &= (1UL << GAME_CACTUS_HIT_BOTTOM) - (1UL << GAME_CACTUS_HIT_TOP);
if ((p >> shift) & c) return 1;
```

`p/c`和列读取的完整定义按快照 `Game_Collides`逐行填写。当前常量使 `shift`非负；先检查 `>=32`也避免超宽右移。透明像素相交不会被算成碰撞。

### 第5步：先判碰撞，再计分重生

在每个逻辑步移动/换帧之后立即碰撞检测，撞到则 `status=GAME_OVER; return;`，不能只在渲染时检测。未撞且障碍完全离开左侧时：

```c
if (game->cactusX + (int16_t)OLED_CACTUS_WIDTH <= 0) {
    if (game->score < GAME_SCORE_MAX) game->score++;
    game->cactusX = 128 + (game->score % 3U) * 12;
}
```

完全通过才加分，障碍移到右外重新出现。速度函数在5分和10分改变档位。

## 验收与边界

在本地完整工程执行 `./tests/run_game_tests.ps1`，覆盖逻辑与渲染；逻辑关注复位、落地、空中禁止再跳、暂停、透明像素、碰撞、计分上限和加速。命令需GCC，测试文件不在本教程快照中。此时不需要真实OLED；板上速度与输入体验留到下一章。
