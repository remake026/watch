#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <stdint.h>

typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
#define GPIOB ((void *)1)
#define GPIO_PIN_15 ((uint16_t)0x8000)

uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
void HAL_GPIO_TogglePin(void *port, uint16_t pin);
void Error_Handler(void);
/* Also supplies the prototype to the pre-migration source used for comparison. */
uint8_t RTC_ReadDateTime(uint8_t value[6]);

#endif
