#include "oled_test_page.h"
#include "main.h"
#include "OLED.h"
#include "key.h"

static void Show_OLEDClockTest(uint8_t failed)
{
    uint8_t clock = OLED_GetDisplayClock();

    OLED_Clear();
    OLED_ShowString(16, 0, "OLED CLOCK TEST", OLED_6X8);
    OLED_ShowString(0, 12, "D5:", OLED_6X8);
    OLED_ShowHexNum(18, 12, clock, 2, OLED_6X8);
    OLED_Printf(48, 12, OLED_6X8, "O:%02u D:%02u",
                (unsigned int)(clock >> 4),
                (unsigned int)((clock & 0x0FU) + 1U));
    OLED_ShowString(0, 26, "开始", OLED_8X16);
    OLED_ShowString(48, 26, "暂停", OLED_8X16);
    OLED_ShowString(96, 26, "重来", OLED_8X16);
    OLED_DrawRectangle(0, 44, 128, 7, OLED_UNFILLED);
    OLED_DrawLine(0, 47, 127, 47);
    if (failed)
    {
        OLED_ShowString(34, 44, "I2C ERROR", OLED_6X8);
    }
    OLED_ShowString(16, 56, "1:-  2:+  3:USE", OLED_6X8);
    OLED_Update();
}

void OLEDTestPage_Run(void)
{
    Show_OLEDClockTest(0);

    while (1)
    {
        uint8_t key = Key_Event();

        if (key == 3)
        {
            return;
        }

        if (key == 1 || key == 2)
        {
            uint8_t oscillator = OLED_GetDisplayClock() >> 4;
            uint8_t clock;
            uint8_t applied;

            if (key == 1)
            {
                oscillator = (oscillator == 0U) ? 15U : oscillator - 1U;
            }
            else
            {
                oscillator = (oscillator + 1U) & 0x0FU;
            }

            /* Sweep the oscillator while keeping divider = 1 on both chips. */
            clock = (uint8_t)(oscillator << 4);
            applied = OLED_SetDisplayClock(clock);
            Show_OLEDClockTest((uint8_t)!applied);
        }

        /* Keep the test image static: no repeated display RAM writes. */
        HAL_Delay(1);
    }
}
