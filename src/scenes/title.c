#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"
#include "game/state.h"

#define STAR_COUNT 6

static sprite_t spr_hero, spr_fx;
static anim_t hero_anim;
static uint8_t blink;
static uint8_t save_loaded;
static int16_t hero_x;                 /* tweened: the keeper walks in from the left */
static int16_t win_y;                  /* tweened: help window slides up */
static char play_label[] = "PLAY LEVEL 00";

static const uint8_t push_frames[] = { 4, 5 };
static const anim_def_t anim_push = { push_frames, 2, 12, 1 };

/* twinkling stars over the walls, drawn directly with spr_put (screen px) */
static const uint8_t star_x[STAR_COUNT] = { 20, 64, 108, 146, 4, 148 };
static const uint8_t star_y[STAR_COUNT] = { 3, 6, 2, 5, 52, 88 };
static const uint8_t star_seq[8] = { 2, 1, 0, 1, 2, 2, 2, 2 };   /* spark frame per phase */

static const char * const menu_options[] = { play_label, "BONUS RUN", "HOW TO PLAY" };

/* level select line (row 14): "< LEVEL 01 >", arrows only where there is somewhere to go */
static void draw_level_select(void) {
    text_print(4, 14, game_level > 0 ? "<" : " ");
    text_print(6, 14, "LEVEL");
    text_print_num(12, 14, game_level + 1, 2);
    text_print(15, 14, game_level < game_unlocked ? ">" : " ");
}

static void draw_band(void) {
    text_clear(2, 12, 16, 4);
    text_print(4, 12, "PRESS START");
    draw_level_select();
    blink = 0;
}

static void slide_window(int16_t to, uint8_t frames, uint8_t ease) {
    tween_start(&win_y, to, frames, ease);
    while (tween_busy(&win_y)) {
        vsync();
        tween_update();
        move_win(7, (uint8_t)win_y);
    }
}

static void show_help(void) {
    box_draw_win(0, 0, 20, 18);
    text_print_win(1, 1, "    HOW TO PLAY\n\n"
                         "SOKOBAN\n"
                         "Push every crate\n"
                         "onto a yellow mark\n"
                         "D-PAD move  B undo\n"
                         "SELECT restart\n"
                         "START  pause\n\n"
                         "BONUS RUN\n"
                         "Grab all coins,\n"
                         "then find the door\n"
                         "A jump (hold=high)\n"
                         "Stomp the rats!\n\n"
                         "      PRESS A");
    win_y = 144;
    move_win(7, 144);
    HIDE_SPRITES;                            /* sprites would draw over the window */
    SHOW_WIN;
    slide_window(0, 30, EASE_INOUT);
    input_wait_press(J_A | J_B | J_START);
    sfx_play(&sfx_menu);
    slide_window(144, 20, EASE_IN);
    HIDE_WIN;
    SHOW_SPRITES;
}

static void open_menu(void) {
    uint8_t choice;
    play_label[11] = '0' + (game_level + 1) / 10;
    play_label[12] = '0' + (game_level + 1) % 10;
    text_clear(2, 12, 16, 4);
    choice = menu_run(3, 12, menu_options, 3);
    if (choice == 2) { show_help(); draw_band(); return; }
    rand_seed();
    sfx_play(&sfx_start);
    scene_goto(choice == 0 ? &scene_level : &scene_bonus, TRANS_FADE_BLACK);
}

static void title_enter(void) {
    if (!save_loaded) {
        save_loaded = 1;
        save_load();
        game_level = game_unlocked;          /* continue where the player left off */
    }
    if (game_level > game_unlocked) game_level = game_unlocked;
    map_load(&map_title, 0);
    cam_set(0, 0);
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    gfx_load_sprite(&spr_fx, &spr_spark, 0);
    text_print(3, 3, "S O K O B A N");
    text_print(2, 4, "WAREHOUSE KEEPER");
    draw_band();
    hero_anim.def = 0;
    anim_play(&hero_anim, &anim_push);
    hero_x = -16;
    tween_start(&hero_x, 32, 50, EASE_OUT);  /* cell (2,4), left of the crate */
    music_play(&mus_title);
}

static void title_update(void) {
    uint8_t i, phase;

    blink++;
    if ((blink & 31) == 0) {
        if (blink & 32) text_clear(4, 12, 11, 1);
        else text_print(4, 12, "PRESS START");
    }

    if (KEY_PRESSED(J_LEFT) && game_level > 0) {
        game_level--; sfx_play(&sfx_menu); draw_level_select();
    } else if (KEY_PRESSED(J_RIGHT) && game_level < game_unlocked) {
        game_level++; sfx_play(&sfx_menu); draw_level_select();
    }

    tween_update();
    anim_update(&hero_anim);
    spr_draw(&spr_hero, anim_frame(&hero_anim), W2S_X(hero_x), W2S_Y(64), 0);
    for (i = 0; i < STAR_COUNT; i++) {
        phase = ((blink >> 3) + i * 3) & 7;
        spr_put(spr_fx.base + star_seq[phase], star_x[i], star_y[i], spr_fx.pal);
    }

    if (KEY_PRESSED(J_START | J_A)) open_menu();
}

const scene_t scene_title = { title_enter, title_update, 0 };
