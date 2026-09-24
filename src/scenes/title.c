#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"
#include "game/state.h"

static sprite_t spr_hero;
static anim_t hero_anim;
static uint8_t blink;
static uint8_t save_loaded;

static const uint8_t push_frames[] = { 4, 5 };
static const anim_def_t anim_push = { push_frames, 2, 12, 1 };

/* level select line (row 13): "< LEVEL 01 >", arrows only where there is somewhere to go */
static void draw_level_select(void) {
    text_print(4, 13, game_level > 0 ? "<" : " ");
    text_print(6, 13, "LEVEL");
    text_print_num(12, 13, game_level + 1, 2);
    text_print(15, 13, game_level < game_unlocked ? ">" : " ");
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
    text_print(3, 3, "S O K O B A N");
    text_print(2, 4, "WAREHOUSE KEEPER");
    text_print(4, 12, "PRESS START");
    draw_level_select();
    hero_anim.def = 0;
    anim_play(&hero_anim, &anim_push);
    blink = 0;
    music_play(&mus_title);
}

static void title_update(void) {
    blink++;
    if ((blink & 31) == 0) text_print(4, 12, (blink & 32) ? "           " : "PRESS START");

    if (KEY_PRESSED(J_LEFT) && game_level > 0) {
        game_level--; sfx_play(&sfx_menu); draw_level_select();
    } else if (KEY_PRESSED(J_RIGHT) && game_level < game_unlocked) {
        game_level++; sfx_play(&sfx_menu); draw_level_select();
    }

    if (KEY_PRESSED(J_START | J_A)) {
        rand_seed();
        sfx_play(&sfx_start);
        scene_goto(&scene_level, TRANS_FADE_BLACK);
    }

    anim_update(&hero_anim);
    spr_draw(&spr_hero, anim_frame(&hero_anim), W2S_X(32), W2S_Y(64), 0);   /* cell (2,4), left of the crate */
}

const scene_t scene_title = { title_enter, title_update, 0 };
