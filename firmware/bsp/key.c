#include "key.h"



static volatile uint8_t key_value = 0;
static uint8_t preKey = 0;
static uint8_t curKey = 0;
static uint8_t cnt = 0;
uint8_t KEY_GetValue(void)
{
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        return 1;
    }
    else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6) == GPIO_PIN_RESET)
    {
        return 2;
    }
    else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4) == GPIO_PIN_RESET)
    {
        return 3;
    }
    return 0;
    
}
void KEY_Scan(void)
{
    cnt++;
    
    if(cnt >= 20)
    {
        cnt = 0;
        preKey = curKey;
        curKey = KEY_GetValue();
        if(preKey != 0 && curKey == 0)
        {
            key_value = preKey;
        }
    
    }
    
}
uint8_t Key_Event(void)
{
    uint32_t primask = __get_PRIMASK();
    uint8_t key;

    __disable_irq();

    key = key_value;
    key_value = 0;

    __set_PRIMASK(primask);

    return key;
}