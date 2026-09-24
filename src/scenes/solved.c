#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"
#include "game/state.h"

#define HOP_GROUND 64
#define HOP_TOP    54

static sprite_t spr_hero, spr_fx;
static uint8_t timer, hops;
static uint8_t last_level;             /* the level just solved was the final one */
static int16_t shown_moves, shown_pushes;   /* tweened: count up from 0 */
static int16_t hop_y;                       /* tweened: the keeper hops happily */

/* sprite, speed, up, gravity, life */
static const particle_style_t st_confetti = { &spr_fx, 24, -40, 3, 36 };

static void solved_enter(void) {
    map_load(&map_title, 0);
    cam_set(0, 0);
    text_set_colors(pal_ui_gold);
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    gfx_load_sprite(&spr_fx, &spr_spark, 0);
    last_level = (game_level + 1 >= LEVEL_COUNT);
    text_print(2, 2, "LEVEL    SOLVED!");
    text_print_num(8, 2, game_level + 1, 2);
    text_print(5, 3, "MOVES");
    text_print(5, 4, "PUSHES");
    text_print(5, 5, game_new_best ? "NEW BEST!" : "BEST");
    if (!game_new_best) text_print_num(12, 5, MIN(game_best[game_level], 999), 3);
    text_print(4, 12, "PRESS START");
    if (last_level) text_print(2, 13, "ALL LEVELS DONE!");
    else {
        text_print(3, 13, "NEXT: LEVEL");
        text_print_num(15, 13, game_level + 2, 2);
    }
    shown_moves = shown_pushes = 0;
    tween_start(&shown_moves, MIN(game_moves, 999), 40, EASE_LINEAR);
    tween_start(&shown_pushes, MIN(game_pushes, 999), 40, EASE_LINEAR);
    hop_y = HOP_GROUND; hops = 0;
    timer = 0;
    music_play(&mus_title);
}

static void solved_leave(void) {
    text_set_colors(pal_ui_default);       /* back to the normal UI colors (screen is faded out) */
}

static void solved_update(void) {
    timer++;
    if ((timer & 31) == 0) {
        if (timer & 32) text_clear(4, 12, 11, 1);
        else text_print(4, 12, "PRESS START");
    }

    if (KEY_PRESSED(J_START | J_A)) {
        sfx_play(&sfx_start);
        if (last_level) scene_goto(&scene_title, TRANS_FADE_BLACK);
        else { game_level++; scene_goto(&scene_level, TRANS_FADE_BLACK); }
    } else if (KEY_PRESSED(J_B)) {
        sfx_play(&sfx_menu);
        scene_goto(&scene_title, TRANS_FADE_BLACK);
    }

    /* the keeper hops (up fast, down accelerating); confetti every 4th landing */
    if (!tween_busy(&hop_y)) {
        if (hop_y == HOP_GROUND) {
            if ((hops++ & 3) == 0) particles_emit(36, HOP_GROUND, 6, &st_confetti);
            tween_start(&hop_y, HOP_TOP, 10, EASE_OUT);
        } else {
            tween_start(&hop_y, HOP_GROUND, 10, EASE_IN);
        }
    }
    tween_update();
    text_print_num(12, 3, shown_moves, 3);
    text_print_num(12, 4, shown_pushes, 3);

    spr_draw(&spr_hero, 0, W2S_X(32), W2S_Y(hop_y), 0);
    particles_update();
}

const scene_t scene_solved = { solved_enter, solved_update, solved_leave };
