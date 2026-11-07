# 09 滑动菜单：把选择、目标与当前位置分开

[模块目录](README.md) · [上一章](08-settings.md) · [下一章](10-output.md)

## 设计思想

选中项可以立即变化，图标位置逐步追上。边框固定在屏幕中央，所有图标一起平移，形成滑动效果。菜单只负责导航；进入秒表或游戏后等待子页返回，再重画菜单。

## 变量与逻辑

| 名称 | 类型/初值 | 功能 |
|---|---|---|
| `menuFlag` | 文件级 `uint8_t`，初值0 | 当前选中编号，返回后保留 |
| `position` | 本次调用 `int32_t` | 当前平移像素，进入时为 `menuFlag*48` |
| `target` | `int32_t` | 本轮目标平移量 `menuFlag*48` |
| `i` | 0～6 | 图标下标 |
| `x` | 有符号坐标 | `48+i*48-position`，可能位于屏幕外 |

| 编号 | 确认动作 |
|---|---|
| 0 | 返回首页 |
| 1 | `StopwatchPage_Run()` |
| 2 | 切换PB15 |
| 3 | 占位，无动作 |
| 4 | `GamePage_Run()` |
| 5 | `AnimationPage_Run()`，当前KUN |
| 6 | `OLEDTestPage_Run()` |

位置每次加/减8，邻项距离48，所以相邻项需要6个移动步骤；每步延时5ms，实际时长还包含I2C发送。内层动画循环不消费按键，但TIM2仍扫描，事件可能在单槽中等待或覆盖。

## A 移植：图标与页面入口分开接入

1. `Menu_Graph`从素材模块取得，确认确实有7张32×32数据。
2. 创建 `menu_page.h/.c`并注册Keil；头文件仅公开 `void MenuPage_Run(void);`。
3. 子页面还没手写时，不要直接加入尚未定义的调用；先完成0返回与2输出，其余入口在相应模块验收后接上。若采用临时空函数，每个只定义一次，随后替换。
4. 当前源码对照 [menu_page.c](reference/bsp/menu_page.c)，[滑动演示](../../demos/menu-slide.html)可在本地浏览器打开理解坐标。

## B 自己编码：静态菜单→滑动→分发

### 第1步：绘制一屏图标

在 `static void ShowMenu(int32_t position)`中，清显存后遍历图标：

```c
for (uint8_t i = 0; i < 7; ++i) {
    int32_t x = 48 + i * 48 - position;
    if (x > -32 && x < 128)
        OLED_ShowImage(x, 16, 32, 32, Menu_Graph[i]);
}
```

例如 `menuFlag=2/position=96`时第2张在X=48；相邻图标在0和96。坐标必须是有符号，负值表示左侧裁切，不用无符号回绕模拟。

### 第2步：补固定边框和标签

边框从 `(42,10)`到 `(85,53)`，用8条短线画四角；编号5写 `KUN DANCE`，编号6写 `CLOCK TEST`。完整坐标看快照的 `ShowMenu`。移动时发送Y16～47图标带和Y56～63标签带；进入菜单与子页返回时额外调用全屏 `OLED_Update`恢复固定边框。

### 第3步：写选择与平移

在页面进入时令 `position=target=menuFlag*48`，画一次完整菜单。循环消费一次按键，键1在大于0时减1，键2在小于6时加1，然后：

```c
target = menuFlag * 48;
while (position != target) {
    if (position < target) position += 8;
    else position -= 8;
    ShowMenu(position);
    HAL_Delay(5);
}
```

当前48是8的整数倍，起止位置也在该网格，所以不会越过目标。改步长后必须加到达/截断判断，不能盲目保留 `!=`条件。

### 第4步：逐个接确认入口

```c
if (key == 3) {
    if (menuFlag == 0) return;
    else if (menuFlag == 1) {
        StopwatchPage_Run();
        ShowMenu(position);
        OLED_Update();
    }
    /* 后续模块按同样方式接入4/5/6；2直接切换输出。 */
}
```

原源码每个子页入口后都恢复菜单，不能只在进菜单时画一次。编号3暂时不添加分支。

## 验收与边界

键1/2到边界停止，无环绕；选择项始终落在中央；进入并退出子页后图标、边框和标签完整，选中编号保留。菜单移动时局部发送的是图标带，若改边框尺寸或增加文字位置，需一并修改刷新区域。
