#include "app.h"
#include "main.h"
#include "tim.h"
#include "OLED.h"
#include "home_page.h"
#include "menu_page.h"
#include "settings_page.h"

/* Shared display/edit cache, owned by the application and passed to pages. */
static uint8_t my_time[6] = {26, 11, 2, 21, 13, 0};

void App_Init(void)
{
    OLED_Init();
    OLED_Clear();

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
}

void App_Run(void)
{
    uint8_t entry = HomePage_Run(my_time);

    if (entry == 1)
    {
        MenuPage_Run();
    }
    else if (entry == 2)
    {
        SettingsPage_Run(my_time);
    }
}
