# 06 应用初始化与调度

[模块目录](README.md) · [上一章](05-rtc.md) · [下一章](07-home.md)

## 设计思想

`main`只处理芯片启动与重复调用，`app.c`负责一次初始化、共享时间缓存与页面选择。`app.c`不再画菜单、不包含游戏碰撞，使每个页面能单独阅读和测试。

## 变量与逻辑

| 变量 | 所有者 | 作用 |
|---|---|---|
| `static uint8_t my_time[6]` | app.c | 首页/设置共享，初值 `{26,11,2,21,13,0}` |
| `uint8_t entry` | 本次App_Run | 保存首页返回的1或2 |
| `htim2` | CubeMX外设模块 | 应用只启动，不重新定义句柄 |

```text
App_Init（一次）：OLED初始化 → 清显存 → 清TIM2更新标志 → 启动中断
App_Run（反复）：首页 → 返回入口1/2 → 菜单/设置 → 返回main
```

`App_Run`本身不需要再写永久循环，`main`已有 `while(1)`。页面返回后下轮再次进入首页。

## A 移植：先用已验收接口

1. OLED、按键、RTC模块单独验收通过后再接应用。
2. 创建 `app.h/.c`并在Keil注册 `.c`；不要复制一份旧大 `app.c`再保留另一套页面实现。
3. 从当前工程重建时，可对照 [app.h](reference/bsp/app.h)、[app.c](reference/bsp/app.c)。这是最终对照，不是要求先把整个功能一次复制进去。

## B 自己编码：先连骨架，再逐页替换

### 第1步：定义公共入口

```c
/* app.h */
#ifndef APP_H
#define APP_H
void App_Init(void);
void App_Run(void);
#endif
```

### 第2步：定义私有时间缓存与初始化

```c
/* app.c：还需包含app.h、main.h、tim.h、OLED.h */
static uint8_t my_time[6] = {26, 11, 2, 21, 13, 0};

void App_Init(void)
{
    OLED_Init();
    OLED_Clear();
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) Error_Handler();
}
```

清更新标志后启动中断，避免把初始化阶段遗留标志当作第一次正常扫描。应用启动完成后不要在页面重复调用这个函数。

### 第3步：解决“页面还没写”问题

先创建 `home_page.h`、`menu_page.h`、`settings_page.h`，接口见总架构。对应 `.c`只写一次临时空壳：首页返回1，菜单/设置暂时直接返回。它们只用于通过链接、验证调用路径；下一章开始逐个替换函数体，不增加第二份同名定义。

### 第4步：写调度

```c
/* app.c还需包含home_page.h、menu_page.h、settings_page.h */
void App_Run(void)
{
    uint8_t entry = HomePage_Run(my_time);
    if (entry == 1) MenuPage_Run();
    else if (entry == 2) SettingsPage_Run(my_time);
}
```

### 第5步：接入main

在用户包含区加 `app.h`；外设初始化后用户区调用一次 `App_Init()`；主循环用户区调用 `App_Run()`。原来用于单独测试OLED和TIM2的临时代码此时移除。在 `USER CODE BEGIN 4`保留上一章的TIM2回调，不能重复定义。

## 验收与边界

先在入口断点确认初始化只执行一次，主循环不断调用调度；填好首页后再测试菜单/设置往返。共享数组不能改成首页的栈临时数组并在返回后继续保存其指针。完整入口对照 [main.c](reference/Core/Src/main.c)。
