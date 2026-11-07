#ifndef OLED_HAL_CONFIG_H
#define OLED_HAL_CONFIG_H

#include "i2c.h"
#include "watch_config.h"
#define OLED_HAL_BUS WATCH_OLED_I2C_HANDLE
#define OLED_HAL_ADDRESS WATCH_OLED_ADDRESS
#define OLED_HAL_COLUMN_OFFSET WATCH_OLED_COLUMN_OFFSET
extern volatile HAL_StatusTypeDef OLED_HAL_LastStatus;
void OLED_SetCursor(uint8_t page, uint8_t x);
void OLED_WriteCommand(uint8_t command);
void OLED_WriteData(uint8_t *data, uint8_t count);

#endif
