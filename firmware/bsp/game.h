#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define GAME_STEP_MS       40U
#define GAME_GROUND_Y      62
#define GAME_PANDA_X        0
#define GAME_PANDA_REST_Y  30
#define GAME_CACTUS_Y      38
#define GAME_SCORE_MAX   9999U

/* Collision bounds inside each bitmap; right/bottom are exclusive. */
#define GAME_PANDA_HIT_LEFT       6
#define GAME_PANDA_HIT_RIGHT     26
#define GAME_PANDA_HIT_TOP        4
#define GAME_PANDA_HIT_BOTTOM    28
#define GAME_CACTUS_HIT_LEFT      3
#define GAME_CACTUS_HIT_RIGHT    13
#define GAME_CACTUS_HIT_TOP       2
#define GAME_CACTUS_HIT_BOTTOM   22

typedef enum
{
    GAME_RUNNING,
    GAME_PAUSED,
    GAME_OVER
} GameStatus;

typedef struct
{
    uint16_t pandaY16; /* Position and velocity in sixteenths of a pixel. */
    int16_t velocity16;
    int16_t cactusX;
    uint16_t score;
    uint8_t frame;
    uint8_t animationTicks;
    GameStatus status;
} GameState;

void Game_Step(GameState *game);
uint8_t Game_Collides(const GameState *game);

static inline void Game_Reset(GameState *game)
{
    game->pandaY16 = GAME_PANDA_REST_Y * 16;
    game->velocity16 = 0;
    game->cactusX = 128;
    game->score = 0;
    game->frame = 0;
    game->animationTicks = 0;
    game->status = GAME_RUNNING;
}

static inline int16_t Game_PandaY(const GameState *game)
{
    return game->pandaY16 / 16U;
}

static inline uint8_t Game_ObstacleSpeed(const GameState *game)
{
    return game->score >= 10U ? 5U : game->score >= 5U ? 4U : 3U;
}

static inline void Game_Jump(GameState *game)
{
    if (game->status == GAME_RUNNING &&
        game->pandaY16 == GAME_PANDA_REST_Y * 16 && game->velocity16 == 0)
        game->velocity16 = -60;
}

static inline void Game_TogglePause(GameState *game)
{
    if (game->status == GAME_RUNNING) game->status = GAME_PAUSED;
    else if (game->status == GAME_PAUSED) game->status = GAME_RUNNING;
}

#endif
