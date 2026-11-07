# 11 秒表：把光标与运行状态分开

[模块目录](README.md) · [上一章](10-output.md) · [下一章](12-game-logic.md)

## 设计思想

选中“暂停”不代表已经暂停；只有按确认才改变运行状态。先用秒级计时理解状态和进位，再独立练习更精确的毫秒累计。当前固件采用静态状态，返回后再次进入会保留累计值与运行标志。

## 变量与逻辑

| 变量 | 类型/作用 | 生命周期 |
|---|---|---|
| `watchT` | 三个 `uint8_t`：hour/minute/sec | 定义显示数据结构 |
| `t1` | 当前累计时分秒，初始0 | 文件级静态，跨页面保留 |
| `wathFlag` | 0返回、1开始、2暂停、3重来 | 静态，但每次进入重置0 |
| `tickRun` | 0暂停、1计时 | 静态，退出不自动清零 |
| `lastTime` | 最近基准毫秒时刻 | 每次进入重设 |

保留源码中的 `wathFlag`拼写以便搜索。`ShowWatchUi`形参 `wathFlaged`当前未使用，实际读取文件级选择变量。

```text
进入：光标0，lastTime=当前tick
每轮：读按键 → 检查距离lastTime是否>=1000ms
到时：更新lastTime；若tickRun为1，sec进位
然后处理上一项/下一项/确认 → 画界面
```

## A 移植：复用按键和显示，不复制定时中断计数

1. 新建 `stopwatch_page.h/.c`，声明 `void StopwatchPage_Run(void);`。
2. 复用 `HAL_GetTick`、中文显示与返回箭头，不另开TIM2计时，TIM2继续只负责按键扫描。
3. 原课程的周期累加方式与旧[第11课](../11-stopwatch.md)的毫秒累计练习都不是当前秒级实现，不能混贴函数和状态变量。
4. 完整当前代码：[stopwatch_page.c](reference/bsp/stopwatch_page.c)。

## B 自己编码：先显示，再走时，再控制

### 第1步：定义数据与画面

```c
typedef struct WatchTime {
    uint8_t hour, minute, sec;
} watchT;
static watchT t1 = {0, 0, 0};
static uint8_t wathFlag = 0;
static uint32_t tickRun = 0, lastTime = 0;
```

写 `ShowWatchUi`：清屏、左上返回箭头、中心 `%02d:%02d:%02d`、底部开始/暂停/重来，按光标反色，最后全屏更新。先用固定数值验收布局。

### 第2步：写当前秒级计时

放在页面循环内、按键动作处理之前：

```c
if (HAL_GetTick() - lastTime >= 1000U) {
    lastTime = HAL_GetTick();
    if (tickRun) {
        if (++t1.sec > 59) {
            t1.sec = 0;
            if (++t1.minute > 59) {
                t1.minute = 0;
                if (++t1.hour > 23) t1.hour = 0;
            }
        }
    }
}
```

暂停时也更新基准，不把整段暂停时间计入。当前每轮最多加1秒，慢循环越过多秒时会丢时间，且每次以now重设基准会累计偏差；这不是高精度秒表。

### 第3步：写选择和动作

键1在0～3内向前，键2向后，到边界停止；键3按下表处理：

| 选择 | 代码动作 |
|---|---|
| 0 | `return`，不改t1/tickRun |
| 1 | 若未运行，设 `tickRun=1`并重设 `lastTime` |
| 2 | `tickRun=0` |
| 3 | 当前tick赋基准、停止、时分秒都清零 |

每轮最后画界面，进入时仅设 `wathFlag=0`与 `lastTime=HAL_GetTick()`。

### 第4步：从菜单1接入

菜单确认后调用秒表函数，返回后重画菜单并全屏更新。不要在秒表内部调用菜单，避免每次返回再叠一层函数调用。

## 验收与边界

测试开始→暂停→等待→开始→清零；59秒和59分进位；退出后再进入。当前离开期间没有页面循环，累计值不增长；若退出前 `tickRun=1`，重新进入后会继续走时，但不补离开期间的时长。旧教程“每次进入归零”不能用于描述当前固件。

后续改进练习：设计 `elapsedMs + lastTick + running`，每轮累计无符号tick差值并转换显示，明确退出后是否后台计时。这是另一个实现，应完整替换计时部分再重新验收。
