#include "menu_page.h"
#include "stopwatch_page.h"
#include "game_page.h"
#include "animation_page.h"
#include "oled_test_page.h"
#include "main.h"
#include "OLED.h"
#include "key.h"

static uint8_t menuFlag = 0;

static void ShowMenu(int32_t position)
{
    OLED_Clear();
    uint8_t i = 0;
    int32_t x = 0;
    for (; i < 7; ++i)
    {
        x = 48 + i * 48 - position;

        if (x > -32 && x < 128)
        {
            OLED_ShowImage(x, 16, 32, 32, Menu_Graph[i]);
        }
    }

    /* Fixed 44x44 selection frame with four open corners. */
    OLED_DrawLine(42, 10, 51, 10); /* Top left. */
    OLED_DrawLine(42, 10, 42, 19);
    OLED_DrawLine(76, 10, 85, 10); /* Top right. */
    OLED_DrawLine(85, 10, 85, 19);
    OLED_DrawLine(42, 53, 51, 53); /* Bottom left. */
    OLED_DrawLine(42, 44, 42, 53);
    OLED_DrawLine(76, 53, 85, 53); /* Bottom right. */
    OLED_DrawLine(85, 44, 85, 53);

    if (menuFlag == 5)
    {
        OLED_ShowString(37, 56, "KUN DANCE", OLED_6X8);
    }
    else if (menuFlag == 6)
    {
        OLED_ShowString(34, 56, "CLOCK TEST", OLED_6X8);
    }

    OLED_UpdateArea(0, 16, 128, 32);
    OLED_UpdateArea(0, 56, 128, 8);
}

void MenuPage_Run(void)
{

    int32_t position = 0;
    int32_t target = 0;

    // 进入时的初始动画
    position = menuFlag * 48;
    target = position;
    ShowMenu(position);
    OLED_Update();
    while (1)
    {
        uint8_t key = Key_Event();
        if (key == 1)
        {
            if (menuFlag > 0)
                menuFlag--;
        }
        else if (key == 2)
        {
            if (menuFlag < 6)
                menuFlag++;
        }
        else if (key == 3)
        {
            if (menuFlag == 0)
            {
                return;
            }
            else if (menuFlag == 1)
            {
                StopwatchPage_Run();
                ShowMenu(position);
                OLED_Update();
            }
            else if (menuFlag == 2)
            {
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
                // return ;
            }
            else if (menuFlag == 4)
            {
                GamePage_Run();
                ShowMenu(position);
                OLED_Update();
            }
            else if (menuFlag == 5)
            {
                AnimationPage_Run();
                ShowMenu(position);
                OLED_Update();
            }
            else if (menuFlag == 6)
            {
                OLEDTestPage_Run();
                ShowMenu(position);
                OLED_Update();
            }
        }

        target = menuFlag * 48;
        while (position != target)
        {
            if (position < target)
                position += 8;
            else
                position -= 8;
            ShowMenu(position);
            HAL_Delay(5);
        }
    }
}
