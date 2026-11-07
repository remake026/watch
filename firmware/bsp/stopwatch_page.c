#include "stopwatch_page.h"
#include "main.h"
#include "OLED.h"
#include "key.h"

typedef struct WatchTime
{
    uint8_t hour;
    uint8_t minute;
    uint8_t sec;

} watchT;

static uint8_t wathFlag = 0;
static watchT t1 = {0, 0, 0};
static uint32_t tickRun = 0;
static uint32_t lastTime = 0;

static void ShowWatchUi(uint8_t wathFlaged)
{
    OLED_Clear();

    OLED_ShowExitArrow(0, 0);
    OLED_Printf(16, 20, OLED_12X24, "%02d:%02d:%02d", t1.hour, t1.minute, t1.sec);
    OLED_ShowString(0, 48, "开始", OLED_8X16);
    OLED_ShowString(48, 48, "暂停", OLED_8X16);
    OLED_ShowString(96, 48, "重来", OLED_8X16);

    if (wathFlag == 0)
    {
        OLED_ReverseArea(0, 0, 24, 24);
    }
    else if (wathFlag == 1)
    {
        OLED_ReverseArea(0, 48, 32, 16);
    }
    else if (wathFlag == 2)
    {
        OLED_ReverseArea(48, 48, 32, 16);
    }
    else if (wathFlag == 3)
    {
        OLED_ReverseArea(96, 48, 32, 16);
    }
    OLED_Update();
}

void StopwatchPage_Run(void)
{
    wathFlag = 0;
    lastTime = HAL_GetTick();
    while (1)
    {
        uint8_t key = Key_Event();
        
        
        if(HAL_GetTick() - lastTime >= 1000)
        {
            lastTime = HAL_GetTick();
            if(tickRun)
            {
                t1.sec++;
                if(t1.sec > 59)
                {
                    t1.sec = 0;
                    t1.minute++;
                    if(t1.minute > 59)
                    {
                        t1.minute = 0;
                        t1.hour++;
                        if(t1.hour > 23)
                        {
                            t1.hour = 0;
                        }
                    }
                }
            }
        }
        if (key == 1)
        {

            if (wathFlag > 0)
                wathFlag--;
        }
        else if (key == 2)
        {
            if (wathFlag < 3)
                wathFlag++;
        }
        else if (key == 3)
        {
            if (wathFlag == 0)
                return;
            else if (wathFlag == 1)
            {
                if(tickRun == 0)
                {
                    tickRun = 1;
                    lastTime = HAL_GetTick();
                }
            }
            else if (wathFlag == 2)
            {
                tickRun = 0;
            }
            else if (wathFlag == 3)
            {
                lastTime = HAL_GetTick();
                tickRun = 0;
                t1.hour = 0;
                t1.minute = 0;
                t1.sec = 0;
            }
        }
        ShowWatchUi(wathFlag);
    }
}
