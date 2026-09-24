#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"
#include "game/state.h"

static sprite_t spr_hero;
static uint8_t timer;
static uint8_t last_level;             /* the level just solved was the final one */

static void solved_enter(void) {
    map_load(&map_title, 0);
    cam_set(0, 0);
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    last_level = (game_level + 1 >= LEVEL_COUNT);
    text_print(2, 2, "LEVEL    SOLVED!");
    text_print_num(8, 2, game_level + 1, 2);
    text_print(5, 3, "MOVES  ");
    text_print_num(12, 3, MIN(game_moves, 999), 3);
    text_print(5, 4, "PUSHES ");
    text_print_num(12, 4, MIN(game_pushes, 999), 3);
    text_print(5, 5, game_new_best ? "NEW BEST!" : "BEST   ");
    if (!game_new_best) text_print_num(12, 5, MIN(game_best[game_level], 999), 3);
    text_print(4, 12, "PRESS START");
    if (last_level) text_print(2, 13, "ALL LEVELS DONE!");
    else {
        text_print(3, 13, "NEXT: LEVEL");
        text_print_num(15, 13, game_level + 2, 2);
    }
    timer = 0;
    music_play(&mus_title);
}

static void solved_update(void) {
    timer++;
    if ((timer & 31) == 0) text_print(4, 12, (timer & 32) ? "           " : "PRESS START");

    if (KEY_PRESSED(J_START | J_A)) {
        sfx_play(&sfx_start);
        if (last_level) scene_goto(&scene_title, TRANS_FADE_BLACK);
        else { game_level++; scene_goto(&scene_level, TRANS_FADE_BLACK); }
    } else if (KEY_PRESSED(J_B)) {
        sfx_play(&sfx_menu);
        scene_goto(&scene_title, TRANS_FADE_BLACK);
    }

    /* the keeper hops happily (facing the player) */
    spr_draw(&spr_hero, 0, W2S_X(32), W2S_Y((timer & 16) ? 62 : 64), 0);
}

const scene_t scene_solved = { solved_enter, solved_update, 0 };
