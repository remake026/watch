#include "animation_page.h"
#include "main.h"
#include "OLED.h"
#include "key.h"

void AnimationPage_Run(void)
{
    uint8_t frame = 0;
    uint32_t lastFrame;

    OLED_Clear();
    OLED_ShowString(37, 0, "KUN DANCE", OLED_6X8);
    OLED_ShowString(0, 0, "3:OUT", OLED_6X8);
    OLED_ShowImage(28, 8, OLED_KUN_DANCE_WIDTH, OLED_KUN_DANCE_HEIGHT,
                   OLED_GetKunDanceFrame(frame));
    OLED_Update();
    lastFrame = HAL_GetTick();

    while (1)
    {
        uint32_t now;

        if (Key_Event() == 3)
        {
            return;
        }

        now = HAL_GetTick();
        if ((uint32_t)(now - lastFrame) >= OLED_KUN_DANCE_FRAME_MS)
        {
            lastFrame = now;
            frame++;
            if (frame >= OLED_KUN_DANCE_FRAMES)
                frame = 0;

            /* Clear the previous pose before drawing the next shoulder shimmy. */
            OLED_ShowImage(28, 8, OLED_KUN_DANCE_WIDTH, OLED_KUN_DANCE_HEIGHT,
                           OLED_GetKunDanceFrame(frame));
            OLED_UpdateArea(28, 8, OLED_KUN_DANCE_WIDTH, OLED_KUN_DANCE_HEIGHT);
        }

        HAL_Delay(1);
    }
}
