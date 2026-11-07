#include "game.h"
#include "OLED.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t OLED_DisplayBuf[8][128];
static uint8_t screen[8][128];
static GameState rendered;
static uint16_t renderedBest;
static uint64_t simTime;
static uint32_t tickBase;
static unsigned checking, fullUpdates, areaUpdates, bytesSent, mode, flags;
static unsigned pausedBytes, pauseChecked, jumpedScore, jumpSent;

uint32_t HAL_GetTick(void) { return tickBase + (uint32_t)simTime; }
void HAL_Delay(uint32_t ms)
{
    simTime += ms;
    if (mode == 4 && !(flags & 4U) && simTime >= 900)
    {
        flags |= 4U;
        simTime += 200; /* Exercise bounded catch-up. */
    }
}
uint8_t Key_Event(void);

void OLED_Update(void)
{
    if (checking) return;
    memcpy(screen, OLED_DisplayBuf, sizeof screen);
    fullUpdates++;
    bytesSent += sizeof screen;
    if (mode == 5) simTime += 100;
}

void OLED_UpdateArea(int16_t x, int16_t y, uint8_t width, uint8_t height)
{
    unsigned bytes = 0;
    assert(!checking && width && height);
    assert(x >= 0 && y >= 0 && x + width <= 128 && y + height <= 64);
    for (int page = y / 8; page <= (y + height - 1) / 8; ++page)
    {
        memcpy(&screen[page][x], &OLED_DisplayBuf[page][x], width);
        bytes += width;
    }
    areaUpdates++;
    bytesSent += bytes;
    if (mode == 5) simTime += (bytes * 9U + 99U) / 100U + 2U;
}

/* Generated directly from the production rendering functions by the runner. */
#include "game_render_impl.inc"

static void check_scene(void)
{
    uint8_t saved[8][128];
    GameState snapshot = rendered;
    uint16_t best = renderedBest;
    memcpy(saved, OLED_DisplayBuf, sizeof saved);
    checking = 1;
    GameDrawScene(&snapshot, best);
    checking = 0;
    assert(memcmp(saved, OLED_DisplayBuf, sizeof saved) == 0);
    assert(memcmp(screen, OLED_DisplayBuf, sizeof screen) == 0);
}

uint8_t Key_Event(void)
{
    check_scene();
    if (simTime >= (mode == 0 || mode == 3 ? 3000U : 10000U)) return 3;
    if (mode == 1)
    {
        if (!(flags & 1U) && simTime >= 200) { flags |= 1U; return 2; }
        if (rendered.status == GAME_PAUSED)
        {
            if (!pauseChecked) { pausedBytes = bytesSent; pauseChecked = 1; }
            assert(bytesSent == pausedBytes);
        }
        if (!(flags & 2U) && simTime >= 1200) { flags |= 2U; return 2; }
    }
    if (mode == 3 && rendered.status == GAME_OVER && !(flags & 1U))
    {
        flags |= 1U;
        return 1;
    }
    if ((mode == 2 || mode == 4 || mode == 5) && rendered.status == GAME_RUNNING)
    {
        if (rendered.score != jumpedScore) { jumpedScore = rendered.score; jumpSent = 0; }
        if (!jumpSent && rendered.velocity16 == 0 &&
            rendered.pandaY16 == GAME_PANDA_REST_Y * 16)
        {
            GameState trial = rendered;
            Game_Jump(&trial);
            for (int i = 0; i < 100 && trial.status == GAME_RUNNING; ++i)
            {
                Game_Step(&trial);
                if (trial.score > rendered.score)
                {
                    jumpSent = 1;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void run(unsigned scenario, uint32_t base)
{
    mode = scenario;
    tickBase = base;
    simTime = checking = fullUpdates = areaUpdates = bytesSent = flags = 0;
    pauseChecked = jumpedScore = jumpSent = 0;
    memset(screen, 0xA5, sizeof screen);
    GamePage_Run();
    check_scene();
    if (mode == 0) assert(rendered.status == GAME_OVER && fullUpdates == 2);
    if (mode == 1) assert(pauseChecked && fullUpdates == 4);
    if (mode == 3) assert((flags & 1U) && fullUpdates == 4);
    if (mode == 2 || mode == 5) assert(rendered.score >= 2 && rendered.status == GAME_RUNNING);
    assert(renderedBest >= rendered.score);
    printf("render mode=%u base=%u score=%u full=%u partial=%u bytes=%u: PASS\n",
           mode, tickBase, rendered.score, fullUpdates, areaUpdates, bytesSent);
}

int main(void)
{
    run(0, 0);
    run(1, 0);
    run(2, 0);
    run(2, UINT32_MAX - 200U);
    run(3, 0);
    run(4, 0);
    run(5, 0);
    puts("All game rendering and page lifecycle tests passed.");
    return 0;
}
