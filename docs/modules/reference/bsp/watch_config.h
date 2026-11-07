#ifndef WATCH_CONFIG_H
#define WATCH_CONFIG_H

/* Change these settings only after checking your actual wiring. */
/* Use 1..10 in the SAME project while following the tutorial. */
#define WATCH_LESSON             10
#if WATCH_LESSON < 1 || WATCH_LESSON > 10
#error WATCH_LESSON_must_be_between_1_and_10
#endif
#define WATCH_KEY1_PORT          GPIOB
#define WATCH_KEY1_PIN           GPIO_PIN_1
#define WATCH_KEY2_PORT          GPIOA
#define WATCH_KEY2_PIN           GPIO_PIN_6
#define WATCH_KEY3_PORT          GPIOA
#define WATCH_KEY3_PIN           GPIO_PIN_4
#define WATCH_LED_PORT           GPIOB
#define WATCH_LED_PIN            GPIO_PIN_15
#define WATCH_LED_ON_LEVEL       GPIO_PIN_RESET
#define WATCH_OLED_I2C_HANDLE    hi2c1
#define WATCH_IMU_I2C_HANDLE     hi2c2
#define WATCH_RTC_HANDLE         hrtc
#define WATCH_ADC_HANDLE         hadc1
#define WATCH_KEY_TIMER_HANDLE   htim2
#define WATCH_KEY_TIMER_INSTANCE TIM2
#define WATCH_OLED_ADDRESS       (0x3CU << 1)
#define WATCH_OLED_COLUMN_OFFSET 0U
/* SSD1306 / SH1106 D5 parameter: upper nibble = oscillator, lower = divider-1.
 * 0xF0 is a high-frequency trial; 0x80 restores this project's previous value.
 * Use the last menu item to compare all 16 oscillator settings on the camera. */
#define WATCH_OLED_DISPLAY_CLOCK 0xF0U
#define WATCH_BATTERY_ENABLED    0
#define WATCH_VDDA_MV            3300U
#define WATCH_DIVIDER_TOP_OHM    10000U
#define WATCH_DIVIDER_BOTTOM_OHM 10000U
#define WATCH_POWER_LATCH_ENABLED 0
#define WATCH_LONG_PRESS_MS      1500U

#endif
