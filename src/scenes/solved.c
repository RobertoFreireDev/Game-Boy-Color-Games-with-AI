#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"
#include "game/state.h"

static sprite_t spr_hero;
static uint8_t timer;

static void solved_enter(void) {
    map_load(&map_title, 0);
    cam_set(0, 0);
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    text_print(3, 2, "PUZZLE SOLVED!");
    text_print(5, 4, "MOVES  ");
    text_print_num(12, 4, MIN(game_moves, 999), 3);
    text_print(5, 5, "PUSHES ");
    text_print_num(12, 5, MIN(game_pushes, 999), 3);
    timer = 0;
    text_print(4, 12, "PRESS START");
    music_play(&mus_title);
}

static void solved_update(void) {
    timer++;
    if ((timer & 31) == 0) text_print(4, 12, (timer & 32) ? "           " : "PRESS START");

    if (KEY_PRESSED(J_START | J_A)) {
        sfx_play(&sfx_menu);
        scene_goto(&scene_title, TRANS_FADE_BLACK);
    }

    /* the keeper hops happily (facing the player) */
    spr_draw(&spr_hero, 0, W2S_X(32), W2S_Y((timer & 16) ? 62 : 64), 0);
}

const scene_t scene_solved = { solved_enter, solved_update, 0 };
