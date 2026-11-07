#include "game.h"
#include "OLED_Data.h"

uint8_t Game_Collides(const GameState *game)
{
    uint8_t cx;
    int16_t pandaY = Game_PandaY(game);
    int16_t shift = GAME_CACTUS_Y - pandaY;
    const uint8_t *panda = Panda_Run[game->frame];

    /* Physics keeps pandaY <= 30, so the cactus origin is always below it. */
    if (shift >= 32) return 0;

    /* Compare whole vertical columns instead of testing every screen pixel. */
    for (cx = GAME_CACTUS_HIT_LEFT; cx < GAME_CACTUS_HIT_RIGHT; ++cx)
    {
        int16_t px = game->cactusX + cx - GAME_PANDA_X;
        uint32_t p, c;
        if ((uint16_t)(px - GAME_PANDA_HIT_LEFT) >= GAME_PANDA_HIT_RIGHT - GAME_PANDA_HIT_LEFT)
            continue;
        p = panda[px] | ((uint32_t)panda[32 + px] << 8) |
            ((uint32_t)panda[64 + px] << 16) | ((uint32_t)panda[96 + px] << 24);
        c = Cactus[cx] | ((uint32_t)Cactus[16 + cx] << 8) |
            ((uint32_t)Cactus[32 + cx] << 16);
        p &= (1UL << GAME_PANDA_HIT_BOTTOM) - (1UL << GAME_PANDA_HIT_TOP);
        c &= (1UL << GAME_CACTUS_HIT_BOTTOM) - (1UL << GAME_CACTUS_HIT_TOP);
        if ((p >> shift) & c) return 1;
    }
    return 0;
}

void Game_Step(GameState *game)
{
    if (game->status != GAME_RUNNING) return;

    game->pandaY16 += game->velocity16;
    game->velocity16 += 5;
    if (game->pandaY16 >= GAME_PANDA_REST_Y * 16)
    {
        game->pandaY16 = GAME_PANDA_REST_Y * 16;
        game->velocity16 = 0;
    }
    game->cactusX -= Game_ObstacleSpeed(game);
    if (++game->animationTicks >= OLED_PANDA_FRAME_MS / GAME_STEP_MS)
    {
        game->animationTicks = 0;
        game->frame = (uint8_t)((game->frame + 1U) % OLED_PANDA_RUN_FRAMES);
    }

    /* Check every physics step, including steps caught up after a slow frame. */
    if (Game_Collides(game))
    {
        game->status = GAME_OVER;
        return;
    }
    if (game->cactusX + (int16_t)OLED_CACTUS_WIDTH <= 0)
    {
        if (game->score < GAME_SCORE_MAX) game->score++;
        game->cactusX = 128 + (game->score % 3U) * 12;
    }
}
