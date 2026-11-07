#include "home_page.h"
#include "main.h"
#include "rtc.h"
#include "OLED.h"
#include "key.h"

#define FIX_YEAR 2000

static uint8_t cliFlag = 1;

static void Show_Ui(const uint8_t time[6])
{

    OLED_Clear();
    OLED_Printf(0, 0, OLED_8X16, "%02d-%02d-%02d",
                FIX_YEAR + time[0],
                time[1], time[2]);
    OLED_Printf(16, 16, OLED_12X24, "%02d:%02d:%02d",
                time[3], time[4],
                time[5]);
    OLED_ShowString(0, 40, "菜单", OLED_12X24);
    OLED_ShowString(80, 40, "设置", OLED_12X24);
    if (cliFlag == 1)
    {
        OLED_ReverseArea(0, 40, 48, 24);
    }
    else
    {
        OLED_ReverseArea(80, 40, 48, 24);
    }
    OLED_Update();
}

uint8_t HomePage_Run(uint8_t time[6])
{
    while (1)
    {
        RTC_ReadDateTime(time);
        uint8_t key = Key_Event();
        if (key == 1)
        {
            cliFlag--;
            if (cliFlag < 1)
            {
                cliFlag = 2;
            }
        }
        else if (key == 2)
        {
            cliFlag++;
            if (cliFlag > 2)
            {
                cliFlag = 1;
            }
        }
        else if (key == 3)
        {
            return cliFlag;
        }
        Show_Ui(time);
    }
}
