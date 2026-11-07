#include "settings_page.h"
#include "main.h"
#include "OLED.h"
#include "key.h"

static uint8_t setFlag = 1;

static void Show_SettingPage_UI(const uint8_t time[6], uint8_t editing)
{

    uint8_t row;

    if (setFlag <= 4)
    {
        OLED_Clear();
        OLED_ShowString(0, 0, "< Back", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "year: %02d", time[0]);
        OLED_Printf(0, 32, OLED_8X16, "mon: %02d", time[1]);
        OLED_Printf(0, 48, OLED_8X16, "day: %02d", time[2]);
        row = setFlag - 1;
    }
    else
    {
        OLED_Clear();
        OLED_Printf(0, 0, OLED_8X16, "hour: %02d", time[3]);
        OLED_Printf(0, 16, OLED_8X16, "min: %02d", time[4]);
        OLED_Printf(0, 32, OLED_8X16, "sec: %02d", time[5]);
        row = setFlag - 5;
    }

    if (editing && setFlag >= 2)
    {
        uint8_t numberX = (setFlag == 2 || setFlag == 5) ? 48 : 40;
        uint8_t numberWidth = (time[setFlag - 2] >= 100) ? 24 : 16;
        OLED_ReverseArea(numberX, row * 16, numberWidth, 16);
    }
    else
    {
        OLED_ReverseArea(0, row * 16, 128, 16);
    }

    OLED_Update();
}

static void SetTime(uint8_t time[6], uint8_t setFlag)
{

    while (1)
    {
        uint8_t key = Key_Event();
        if (setFlag <= 4)
        {
            if (key == 1)
            {
                time[setFlag - 2]--;
            }
            else if (key == 2)
            {
                time[setFlag - 2]++;
            }
            else if (key == 3)
            {
                return;
            }
        }
        else
        {
            if (key == 1)
            {
                time[setFlag - 2]--;
                if (time[setFlag - 2] < 0)
                {
                    time[setFlag - 2] = 0;
                }
            }
            else if (key == 2)
            {
                time[setFlag - 2]++;
                if (time[setFlag - 2] > 59)
                {
                    time[setFlag - 2] = 0;
                }
            }
            else if (key == 3)
            {
                return;
            }
        }

        Show_SettingPage_UI(time, 1);
    }
}

void SettingsPage_Run(uint8_t time[6])
{
    setFlag = 1;
    while (1)
    {
        uint8_t key = Key_Event();
        if (key == 1)
        {
            if (setFlag > 1)
                setFlag--;
            else
                setFlag = 7;
        }
        else if (key == 2)
        {
            if (setFlag < 7)
                setFlag++;
            else
                setFlag = 1;
        }
        else if (key == 3)
        {
            if (setFlag == 1)
            {
                return;
            }
            else
            {
                SetTime(time, setFlag);
            }
        }

        Show_SettingPage_UI(time, 0);
    }
}
