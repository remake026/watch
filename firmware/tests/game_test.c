#include "game.h"
#include "OLED_Data.h"
#include <assert.h>
#include <stdio.h>

static uint8_t reference_collision(const GameState *game, int inset)
{
    int x, y;
    for (y = 0; y < (int)OLED_PANDA_HEIGHT; ++y)
        for (x = 0; x < (int)OLED_PANDA_WIDTH; ++x)
        {
            int cx = GAME_PANDA_X + x - game->cactusX;
            int cy = Game_PandaY(game) + y - GAME_CACTUS_Y;
            if (inset && (x < GAME_PANDA_HIT_LEFT || x >= GAME_PANDA_HIT_RIGHT ||
                          y < GAME_PANDA_HIT_TOP || y >= GAME_PANDA_HIT_BOTTOM ||
                          cx < GAME_CACTUS_HIT_LEFT || cx >= GAME_CACTUS_HIT_RIGHT ||
                          cy < GAME_CACTUS_HIT_TOP || cy >= GAME_CACTUS_HIT_BOTTOM))
                continue;
            if (cx >= 0 && cx < (int)OLED_CACTUS_WIDTH &&
                cy >= 0 && cy < (int)OLED_CACTUS_HEIGHT &&
                (Panda_Run[game->frame][y / 8 * OLED_PANDA_WIDTH + x] & (1U << (y % 8))) &&
                (Cactus[cy / 8 * OLED_CACTUS_WIDTH + cx] & (1U << (cy % 8))))
                return 1;
        }
    return 0;
}

static void test_collision(void)
{
    GameState game;
    int frame, y, x;
    unsigned transparentOverlaps = 0, forgiven = 0, bodyHits = 0;
    Game_Reset(&game);
    for (frame = 0; frame < (int)OLED_PANDA_RUN_FRAMES; ++frame)
        for (y = 5; y <= GAME_PANDA_REST_Y; ++y)
            for (x = -16; x <= 32; ++x)
            {
                game.frame = (uint8_t)frame;
                game.pandaY16 = (int16_t)(y * 16);
                game.cactusX = (int16_t)x;
                uint8_t hit = Game_Collides(&game);
                uint8_t original = reference_collision(&game, 0);
                assert(hit == reference_collision(&game, 1));
                assert(!hit || original); /* Insets must never create new collisions. */
                if (original && !hit) forgiven++;
                if (hit) bodyHits++;
                if (x > -16 && x < 32 && !Game_Collides(&game)) transparentOverlaps++;
            }
    assert(transparentOverlaps > 0);
    assert(forgiven > 0 && bodyHits > 0);
    printf("collision inset: %u edge contacts forgiven, %u body hits retained\n", forgiven, bodyHits);
}

static void test_jump_pause_and_end(void)
{
    GameState game, saved;
    int ticks = 0;
    Game_Reset(&game);
    Game_Jump(&game);
    assert(game.velocity16 < 0);
    Game_Step(&game);
    saved = game;
    Game_Jump(&game); /* No double jump. */
    assert(game.velocity16 == saved.velocity16);
    Game_TogglePause(&game);
    saved = game;
    for (int i = 0; i < 100; ++i) { Game_Step(&game); Game_Jump(&game); }
    assert(game.pandaY16 == saved.pandaY16 && game.cactusX == saved.cactusX);
    assert(game.velocity16 == saved.velocity16 && game.frame == saved.frame);
    Game_TogglePause(&game);
    while (game.pandaY16 != GAME_PANDA_REST_Y * 16 && ticks++ < 30)
    {
        Game_Step(&game);
        assert(Game_PandaY(&game) >= 5); /* The whole visible character stays on screen. */
    }
    assert(ticks < 30 && game.velocity16 == 0);
    Game_Reset(&game);
    for (int i = 0; i < 100 && game.status == GAME_RUNNING; ++i) Game_Step(&game);
    assert(game.status == GAME_OVER && game.score == 0);
    saved = game;
    Game_TogglePause(&game);
    Game_Jump(&game);
    Game_Step(&game);
    assert(game.status == GAME_OVER && game.cactusX == saved.cactusX);
    Game_Reset(&game);
    assert(game.status == GAME_RUNNING && game.score == 0 && game.cactusX == 128);
    assert(game.frame == 0 && game.velocity16 == 0);
}

static void test_playable_jumps(void)
{
    for (int level = 0; level < 3; ++level)
    {
        unsigned successes = 0;
        int first = -1, last = -1;
        for (int launch = 1; launch <= 60; ++launch)
        {
            GameState game;
            Game_Reset(&game);
            game.score = (uint16_t)(level * 5);
            for (int step = 1; step <= 80 && game.status == GAME_RUNNING; ++step)
            {
                if (step == launch) Game_Jump(&game);
                Game_Step(&game);
                if (game.score > level * 5)
                {
                    successes++;
                    if (first < 0) first = launch;
                    last = launch;
                    break;
                }
            }
        }
        printf("speed %d: %u successful jump timings (steps %d..%d)\n", level + 3, successes, first, last);
        assert(successes >= 2); /* More than a single precisely timed input. */
    }
}

static void test_scoring_and_speed(void)
{
    GameState game;
    Game_Reset(&game);
    game.cactusX = -16;
    Game_Step(&game);
    assert(game.score == 1 && game.cactusX >= 128 && game.status == GAME_RUNNING);
    Game_Step(&game);
    assert(game.score == 1); /* The respawned obstacle must not score again. */
    game.score = 4; assert(Game_ObstacleSpeed(&game) == 3);
    game.score = 5; assert(Game_ObstacleSpeed(&game) == 4);
    game.score = 10; assert(Game_ObstacleSpeed(&game) == 5);
    game.score = GAME_SCORE_MAX;
    game.cactusX = -16;
    Game_Step(&game);
    assert(game.score == GAME_SCORE_MAX && Game_ObstacleSpeed(&game) == 5);
}

static void test_repeated_obstacles(void)
{
    GameState game;
    Game_Reset(&game);
    for (int obstacle = 0; obstacle < 20; ++obstacle)
    {
        int passed = 0;
        for (int launch = 1; launch <= 70 && !passed; ++launch)
        {
            GameState trial = game;
            for (int step = 1; step <= 90 && trial.status == GAME_RUNNING; ++step)
            {
                if (step == launch) Game_Jump(&trial);
                Game_Step(&trial);
                if (trial.score > game.score)
                {
                    game = trial;
                    passed = 1;
                    break;
                }
            }
        }
        assert(passed);
    }
    assert(game.score == 20 && Game_ObstacleSpeed(&game) == 5);
}

int main(void)
{
    test_collision();
    test_jump_pause_and_end();
    test_playable_jumps();
    test_scoring_and_speed();
    test_repeated_obstacles();
    puts("All game logic tests passed.");
    return 0;
}
