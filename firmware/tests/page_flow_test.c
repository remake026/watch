#include "main.h"
#include "tim.h"
#include "app.h"
#include "OLED.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint32_t tick; uint8_t key; } Event;
static Event events[256];
static unsigned eventCount, eventIndex, scenario, gpioToggles;
static uint32_t tick, eventTick, trace = 2166136261U, calls;
static uint8_t displayClock = 0xF0;
static unsigned fields[6];
static char watchTime[24];
TIM_HandleTypeDef htim2;

static void record(const char *format, ...)
{
    char message[256];
    va_list args;
    va_start(args, format);
    int size = vsnprintf(message, sizeof message, format, args);
    va_end(args);
    assert(size >= 0 && size < (int)sizeof message);
    for (int i = 0; i < size; ++i) trace = (trace ^ (uint8_t)message[i]) * 16777619U;
    calls++;
}

static void push(uint8_t key, uint32_t elapsed)
{
    assert(eventCount < sizeof events / sizeof events[0]);
    eventTick += elapsed;
    events[eventCount++] = (Event){eventTick, key};
}

static void press(uint8_t key) { push(key, 40); }
static void open_menu(unsigned item)
{
    press(0); /* Render the home page before confirming. */
    press(3);
    for (unsigned i = 0; i < item; ++i) press(2);
    if (item) press(3);
}
static void close_menu(unsigned item)
{
    for (unsigned i = 0; i < item; ++i) press(1);
    press(3);
}

uint8_t Key_Event(void)
{
    assert(eventIndex < eventCount);
    tick = events[eventIndex].tick;
    return events[eventIndex++].key;
}
uint32_t HAL_GetTick(void) { return tick; }
void HAL_Delay(uint32_t ms) { tick += ms; record("delay:%u;", ms); }
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *handle)
{ (void)handle; record("timer;"); return HAL_OK; }
void HAL_GPIO_TogglePin(void *port, uint16_t pin)
{ assert(port == GPIOB && pin == GPIO_PIN_15); gpioToggles++; record("toggle;"); }
void Error_Handler(void) { abort(); }
uint8_t RTC_ReadDateTime(uint8_t value[6])
{
    const uint8_t clock[6] = {26, 11, 2, 21, 13, (uint8_t)((tick / 1000U) % 60U)};
    memcpy(value, clock, sizeof clock);
    record("rtc:%u;", tick);
    return 1;
}

void OLED_Init(void) { record("init;"); }
void OLED_Clear(void) { record("clear;"); }
void OLED_Update(void) { record("update;"); }
void OLED_UpdateArea(int16_t x, int16_t y, uint8_t w, uint8_t h)
{ record("updateArea:%d,%d,%u,%u;", x, y, w, h); }
void OLED_ClearArea(int16_t x, int16_t y, uint8_t w, uint8_t h)
{ record("clearArea:%d,%d,%u,%u;", x, y, w, h); }
void OLED_ReverseArea(int16_t x, int16_t y, uint8_t w, uint8_t h)
{ record("reverse:%d,%d,%u,%u;", x, y, w, h); }
void OLED_ShowString(int16_t x, int16_t y, char *s, uint8_t font)
{ record("string:%d,%d,%u,%s;", x, y, font, s); }
void OLED_Printf(int16_t x, int16_t y, uint8_t font, char *format, ...)
{
    char message[128];
    va_list args;
    va_start(args, format);
    int size = vsnprintf(message, sizeof message, format, args);
    va_end(args);
    assert(size >= 0 && size < (int)sizeof message);
    const char *names[6] = {"year:", "mon:", "day:", "hour:", "min:", "sec:"};
    for (unsigned i = 0; i < 6; ++i)
        if (strncmp(message, names[i], strlen(names[i])) == 0)
            assert(sscanf(message + strlen(names[i]), "%u", &fields[i]) == 1);
    if (font == OLED_12X24 && x == 16 && y == 20) strcpy(watchTime, message);
    record("printf:%d,%d,%u,%s;", x, y, font, message);
}
void OLED_ShowImage(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *data)
{
    uint32_t sum = 0;
    for (unsigned i = 0; i < w * ((h + 7U) / 8U); ++i) sum = sum * 31U + data[i];
    record("image:%d,%d,%u,%u,%u;", x, y, w, h, sum);
}
void OLED_DrawLine(int16_t x, int16_t y, int16_t x1, int16_t y1)
{ record("line:%d,%d,%d,%d;", x, y, x1, y1); }
void OLED_DrawPoint(int16_t x, int16_t y) { record("point:%d,%d;", x, y); }
void OLED_DrawRectangle(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t filled)
{ record("rectangle:%d,%d,%u,%u,%u;", x, y, w, h, filled); }
void OLED_ShowExitArrow(int16_t x, int16_t y) { record("exit:%d,%d;", x, y); }
void OLED_ShowHexNum(int16_t x, int16_t y, uint32_t value, uint8_t length, uint8_t font)
{ record("hex:%d,%d,%u,%u,%u;", x, y, value, length, font); }
uint8_t OLED_GetDisplayClock(void) { return displayClock; }
uint8_t OLED_SetDisplayClock(uint8_t clock)
{ displayClock = clock; record("clock:%u;", clock); return 1; }
void OLED_ShowPandaRun(int16_t x, int16_t y, uint8_t frame)
{ record("panda:%d,%d,%u;", x, y, frame); }

int main(int argc, char **argv)
{
    assert(argc == 2);
    scenario = (unsigned)atoi(argv[1]);
    switch (scenario)
    {
        case 0: /* Home selection in both directions, menu back. */
            press(0); press(2); press(1); press(3); press(3);
            break;
        case 1: /* Edit each of the six fields and return. */
            press(2); press(3);
            for (unsigned field = 0; field < 6; ++field)
            {
                press(2); press(3); press(field == 2 ? 1 : 2); press(3);
            }
            press(2); press(3);
            break;
        case 2: /* Running stopwatch retains its state across page re-entry. */
            open_menu(1);
            press(2); press(3); push(0, 1100); press(1); press(3);
            press(3); push(0, 1100); press(3);
            close_menu(1);
            break;
        case 3:
            open_menu(2); press(3); close_menu(2);
            break;
        case 4:
            open_menu(5); push(0, 80); push(0, 80); press(3); close_menu(5);
            break;
        case 5:
            open_menu(6); press(1); press(2); press(2); press(3); close_menu(6);
            break;
        case 6:
            open_menu(4); push(0, 80); press(1); push(0, 80); press(3); close_menu(4);
            break;
        case 7: /* Unimplemented menu slot remains inert. */
            open_menu(3); close_menu(3);
            break;
        case 8: /* Stopwatch pause and clear. */
            open_menu(1); press(2); press(3); push(0, 1100);
            press(2); press(3); push(0, 2000); press(2); press(3);
            press(1); press(1); press(1); press(3); close_menu(1);
            break;
        default: abort();
    }
    App_Init();
    App_Run();
    assert(eventIndex == eventCount);
    if (scenario == 1)
    {
        const unsigned expected[6] = {27, 12, 1, 22, 14, 1};
        assert(memcmp(fields, expected, sizeof fields) == 0);
    }
    if (scenario == 2) assert(strcmp(watchTime, "00:00:02") == 0);
    if (scenario == 3) assert(gpioToggles == 2);
    if (scenario == 5) assert(displayClock == 0);
    if (scenario == 8) assert(strcmp(watchTime, "00:00:00") == 0);
    printf("page scenario=%u events=%u calls=%u trace=%08x: PASS\n", scenario, eventIndex, calls, trace);
    return 0;
}
