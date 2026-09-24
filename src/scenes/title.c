#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"

static sprite_t spr_hero;
static anim_t hero_anim;
static uint8_t blink;

static const uint8_t push_frames[] = { 4, 5 };
static const anim_def_t anim_push = { push_frames, 2, 12, 1 };

static void title_enter(void) {
    map_load(&map_title, 0);
    cam_set(0, 0);
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    text_print(3, 3, "S O K O B A N");
    text_print(2, 4, "WAREHOUSE KEEPER");
    text_print(4, 12, "PRESS START");
    hero_anim.def = 0;
    anim_play(&hero_anim, &anim_push);
    blink = 0;
    music_play(&mus_title);
}

static void title_update(void) {
    blink++;
    if ((blink & 31) == 0) text_print(4, 12, (blink & 32) ? "           " : "PRESS START");

    if (KEY_PRESSED(J_START | J_A)) {
        rand_seed();
        sfx_play(&sfx_start);
        scene_goto(&scene_level, TRANS_FADE_BLACK);
    }

    anim_update(&hero_anim);
    spr_draw(&spr_hero, anim_frame(&hero_anim), W2S_X(32), W2S_Y(64), 0);   /* cell (2,4), left of the crate */
}

const scene_t scene_title = { title_enter, title_update, 0 };
