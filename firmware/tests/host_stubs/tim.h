#ifndef TEST_TIM_H
#define TEST_TIM_H

#include "main.h"
typedef struct { void *Instance; } TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim2;
#define TIM_FLAG_UPDATE 1U
#define __HAL_TIM_CLEAR_FLAG(handle, flag) ((void)(handle), (void)(flag))
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *handle);

#endif
