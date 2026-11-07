#include "game_page.h"
#include "game.h"
#include "main.h"
#include "OLED.h"
#include "key.h"

/* Clip update regions before passing them to the OLED driver. */
static void GameUpdateArea(int16_t x, int16_t y, int16_t width, int16_t height)
{
    int16_t right = x + width;
    int16_t bottom = y + height;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right > 128) right = 128;
    if (bottom > 64) bottom = 64;
    if (right > x && bottom > y)
        OLED_UpdateArea(x, y, (uint8_t)(right - x), (uint8_t)(bottom - y));
}

static void GameDrawSprites(const GameState *game)
{
    uint8_t x, y;
    OLED_ShowPandaRun(GAME_PANDA_X, Game_PandaY(game), game->frame);
    /* Add lit cactus pixels without clearing the overlapping panda. */
    for (y = 0; y < OLED_CACTUS_HEIGHT; ++y)
        for (x = 0; x < OLED_CACTUS_WIDTH; ++x)
            if (Cactus[(y / 8U) * OLED_CACTUS_WIDTH + x] & (1U << (y % 8U)))
                OLED_DrawPoint(game->cactusX + x, GAME_CACTUS_Y + y);
}

static void GameDrawScore(const GameState *game, uint16_t best)
{
    OLED_ClearArea(0, 0, 128, 8);
    OLED_Printf(0, 0, OLED_6X8, "S:%04u HI:%04u",
                (unsigned int)game->score, (unsigned int)best);
}

static void GameDrawScene(const GameState *game, uint16_t best)
{
    OLED_Clear();
    GameDrawSprites(game);
    OLED_DrawLine(0, GAME_GROUND_Y, 127, GAME_GROUND_Y);
    GameDrawScore(game, best);
    if (game->status != GAME_RUNNING)
    {
        OLED_ClearArea(0, 16, 128, 24);
        OLED_ShowString(28, 24, game->status == GAME_OVER ? "OVER 1:RETRY" : "PAUSED 2:GO", OLED_6X8);
    }
    OLED_Update();
}

static void GameDrawChanges(const GameState *old, const GameState *game, uint16_t best)
{
    int16_t oldY = Game_PandaY(old);
    int16_t pandaY = Game_PandaY(game);

    OLED_ClearArea(GAME_PANDA_X, oldY, OLED_PANDA_WIDTH, OLED_PANDA_HEIGHT);
    OLED_ClearArea(old->cactusX, GAME_CACTUS_Y, OLED_CACTUS_WIDTH, OLED_CACTUS_HEIGHT);
    GameDrawSprites(game);
    /* The sprite's transparent top rows can clear the HUD near the jump apex. */
    if (old->score != game->score || oldY < 8 || pandaY < 8)
        GameDrawScore(game, best);

    if (oldY != pandaY || old->frame != game->frame)
    {
        int16_t top = oldY < pandaY ? oldY : pandaY;
        int16_t bottom = (oldY > pandaY ? oldY : pandaY) + OLED_PANDA_HEIGHT;
        GameUpdateArea(GAME_PANDA_X, top, OLED_PANDA_WIDTH, bottom - top);
    }
    if (game->cactusX <= old->cactusX)
    {
        GameUpdateArea(game->cactusX, GAME_CACTUS_Y,
                       old->cactusX - game->cactusX + OLED_CACTUS_WIDTH, OLED_CACTUS_HEIGHT);
    }
    else
    {
        GameUpdateArea(old->cactusX, GAME_CACTUS_Y, OLED_CACTUS_WIDTH, OLED_CACTUS_HEIGHT);
        GameUpdateArea(game->cactusX, GAME_CACTUS_Y, OLED_CACTUS_WIDTH, OLED_CACTUS_HEIGHT);
    }
    if (old->score != game->score || oldY < 8 || pandaY < 8)
    {
        GameUpdateArea(0, 0, 128, 8);
    }
}

void GamePage_Run(void)
{
    static uint16_t best = 0;
    GameState game;
    uint32_t lastStep;

    Game_Reset(&game);
    GameDrawScene(&game, best);
    lastStep = HAL_GetTick();

    while (1)
    {
        uint8_t key = Key_Event();
        uint32_t now = HAL_GetTick();
        uint32_t steps;
        GameState old = game;

        if (key == 3) return;
        if (key == 1)
        {
            if (game.status == GAME_OVER)
            {
                Game_Reset(&game);
                GameDrawScene(&game, best);
                lastStep = HAL_GetTick();
                continue;
            }
            Game_Jump(&game);
        }
        else if (key == 2)
        {
            Game_TogglePause(&game);
            if (old.status != game.status)
            {
                GameDrawScene(&game, best);
                lastStep = HAL_GetTick();
                continue;
            }
        }

        if (game.status != GAME_RUNNING || (uint32_t)(now - lastStep) < GAME_STEP_MS)
        {
            HAL_Delay(1);
            continue;
        }

        steps = (uint32_t)(now - lastStep) / GAME_STEP_MS;
        lastStep += steps * GAME_STEP_MS;
        if (steps > 4U) steps = 4U;
        while (steps-- > 0U && game.status == GAME_RUNNING)
        {
            Game_Step(&game);
        }
        if (game.score > best) best = game.score;
        if (game.status == GAME_OVER) GameDrawScene(&game, best);
        else GameDrawChanges(&old, &game, best);
    }
}
