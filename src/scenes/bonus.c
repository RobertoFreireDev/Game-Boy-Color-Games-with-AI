#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"

/* Bonus run: a small side-scrolling platformer through the warehouse.
   Collect every coin to open the exit door, stomp rats, avoid spikes and the pit. */

#define ST_PLAY 0
#define ST_DOOR 1              /* walking into the open exit door */
#define ST_DEAD 2

#define MAX_COINS 12
#define MAX_RATS 4
#define RAT_OFF   0
#define RAT_WALK  1
#define RAT_DYING 2

#define GRAVITY    4
#define MAX_FALL   FIX(4)
#define JUMP_VY    (-FIX(4))   /* about 34 px high with GRAVITY 4 */
#define BOUNCE_VY  (-FIX(3))   /* after stomping a rat */
#define RUN_SPEED  24          /* 1.5 px per frame */
#define RUN_ACCEL  2
#define RUN_DECEL  3
#define RAT_SPEED  8           /* 0.5 px per frame */
#define START_HP   3
#define INVULN_FRAMES 60

/* hero hitbox 10x14 inside the 16x16 sprite */
#define HERO_OX 3
#define HERO_OY 2

typedef struct { int16_t x, y; uint8_t active; } coin_t;          /* world px */
typedef struct { body_t b; uint8_t state, right; } rat_t;

static sprite_t spr_hero, spr_c, spr_r, spr_fx;
static anim_t hero_anim, coin_anim, rat_anim;
static body_t hero;
static coin_t coins[MAX_COINS];
static rat_t rats[MAX_RATS];
static uint8_t coin_count, coins_got;
static uint8_t hp, invuln, state, timer, facing_left, door_open;
static uint16_t door_tx, door_ty;
static int16_t safe_x, safe_y;         /* last position standing safely on the ground (fixed) */

/* player frames: 2 up (back), 4 right, 5 right-step; left = SPR_FLIPX */
static const uint8_t fr_idle[] = { 4 }, fr_run[] = { 4, 5 }, fr_jump[] = { 5 }, fr_back[] = { 2 };
static const anim_def_t anim_idle = { fr_idle, 1, 1, 1 };
static const anim_def_t anim_run  = { fr_run, 2, 6, 1 };
static const anim_def_t anim_jump = { fr_jump, 1, 1, 1 };
static const anim_def_t anim_back = { fr_back, 1, 1, 1 };
static const uint8_t fr_coin[] = { 0, 1, 2, 3 };
static const anim_def_t anim_coin = { fr_coin, 4, 8, 1 };
static const uint8_t fr_rat[] = { 0, 1 };
static const anim_def_t anim_rat = { fr_rat, 2, 10, 1 };

/* sprite, speed, up, gravity, life */
static const particle_style_t st_sparkle = { &spr_fx, 20, -16, 1, 24 };
static const particle_style_t st_dust    = { &spr_fx, 14, -8, 0, 15 };

static const char * const hp_str[] = { "   ", "*  ", "** ", "***" };
static const char * const pause_options[] = { "CONTINUE", "QUIT TO TITLE" };

static void hud_update(void) {
    text_print_num_win(6, 0, coins_got, 2);
    text_print_win(17, 0, hp_str[hp]);
}

static void hud_show(void) {
    text_print_win(0, 0, "COINS 00/00   HP    ");
    text_print_num_win(9, 0, coin_count, 2);
    hud_update();
    move_win(7, 136);
    SHOW_WIN;
}

static void bonus_enter(void) {
    uint16_t tx, ty;
    rat_t *r;

    map_load(&map_bonus, 0);
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    gfx_load_sprite(&spr_c, &spr_coin, 0);
    gfx_load_sprite(&spr_r, &spr_rat, 0);
    gfx_load_sprite(&spr_fx, &spr_spark, 0);

    /* objects are markers in the map (TF_SPAWN), found by character */
    map_find('P', 0, &tx, &ty);
    hero.x = FIX((tx << 3) + HERO_OX); hero.y = FIX((ty << 3) + HERO_OY);
    hero.vx = hero.vy = 0; hero.w = 10; hero.h = 14;
    safe_x = hero.x; safe_y = hero.y;

    for (coin_count = 0; coin_count < MAX_COINS && map_find('o', coin_count, &tx, &ty); coin_count++) {
        coins[coin_count].x = tx << 3; coins[coin_count].y = ty << 3;
        coins[coin_count].active = 1;
    }
    for (r = rats; r < rats + MAX_RATS; r++) {
        r->state = RAT_OFF;
        if (!map_find('R', (uint8_t)(r - rats), &tx, &ty)) continue;
        r->b.x = FIX((tx << 3) + 1); r->b.y = FIX(ty << 3);
        r->b.vx = r->b.vy = 0; r->b.w = 14; r->b.h = 8;
        r->state = RAT_WALK; r->right = 0;
    }
    map_find('H', 0, &door_tx, &door_ty);

    coins_got = 0; hp = START_HP; invuln = 0; door_open = 0; facing_left = 0;
    state = ST_PLAY; timer = 0;
    hero_anim.def = 0; coin_anim.def = 0; rat_anim.def = 0;
    anim_play(&hero_anim, &anim_idle);
    anim_play(&coin_anim, &anim_coin);
    anim_play(&rat_anim, &anim_rat);

    cam_set(UNFIX(hero.x) - 75, UNFIX(hero.y) - 65);
    hud_show();
    music_play(&mus_bonus);
}

static void hurt(void) {
    if (invuln || state != ST_PLAY) return;
    sfx_play(&sfx_hurt);
    cam_shake(12, 2);
    if (hp) hp--;
    hud_update();
    if (!hp) {                                  /* classic death: hop up, fall upside down */
        state = ST_DEAD; timer = 0;
        hero.vy = -FIX(3);
        music_play(&mus_gameover);
        return;
    }
    invuln = INVULN_FRAMES;
    hero.vy = -FIX(2);
    hero.vx = facing_left ? FIX(1) : -FIX(1);  /* knocked backwards */
}

static void open_door(void) {
    door_open = 1;
    sfx_play(&sfx_door);
    cam_shake(20, 1);
}

/* map_set_tile only changes VRAM; streaming redraws the closed door from the map rows,
   so the open door is re-applied every frame (off-screen calls are ignored). */
static void door_refresh(void) {
    if (!door_open) return;
    map_set_tile(door_tx,     door_ty,     'K');
    map_set_tile(door_tx + 1, door_ty,     'k');
    map_set_tile(door_tx,     door_ty + 1, 'L');
    map_set_tile(door_tx + 1, door_ty + 1, 'l');
}

static void update_rats(void) {
    rat_t *r;
    int16_t fx, fy;
    uint8_t hit;
    for (r = rats; r < rats + MAX_RATS; r++) {
        if (r->state == RAT_DYING) {            /* no collision: falls off the map */
            r->b.vy += GRAVITY;
            r->b.y += r->b.vy;
            if (UNFIX(r->b.y) > (int16_t)map_h_px) r->state = RAT_OFF;
            continue;
        }
        if (r->state != RAT_WALK) continue;
        r->b.vx = r->right ? RAT_SPEED : -RAT_SPEED;
        r->b.vy += GRAVITY;
        if (r->b.vy > MAX_FALL) r->b.vy = MAX_FALL;
        hit = body_move(&r->b);
        if (hit & (HIT_LEFT | HIT_RIGHT)) {
            r->right ^= 1;
        } else if (body_on_ground(&r->b)) {     /* turn at ledges and in front of spikes */
            fx = r->right ? UNFIX(r->b.x) + r->b.w : UNFIX(r->b.x) - 1;
            fy = UNFIX(r->b.y) + r->b.h;
            if (!(map_flags((uint16_t)fx >> 3, (uint16_t)fy >> 3) & (TF_SOLID | TF_ONEWAY))
                || (map_flags_px(fx, fy - 1) & TF_HAZARD))
                r->right ^= 1;
        }

        if (!body_overlap(&hero, &r->b)) continue;
        if (hero.vy > 0 && UNFIX(hero.y) + hero.h - 1 < UNFIX(r->b.y) + 4) {    /* landed on top */
            r->state = RAT_DYING;
            r->b.vy = -FIX(2);
            hero.vy = BOUNCE_VY;
            particles_emit(UNFIX(r->b.x) + 4, UNFIX(r->b.y), 4, &st_dust);
            sfx_play(&sfx_stomp);
        } else {
            hurt();
        }
    }
}

static void update_coins(void) {
    coin_t *c;
    int16_t hx = UNFIX(hero.x), hy = UNFIX(hero.y);
    for (c = coins; c < coins + coin_count; c++) {
        if (!c->active || !rect_overlap(hx, hy, hero.w, hero.h, c->x, c->y, 8, 8)) continue;
        c->active = 0;
        coins_got++;
        particles_emit(c->x, c->y, 6, &st_sparkle);
        sfx_play(&sfx_coin);
        hud_update();
        if (coins_got == coin_count) open_door();
    }
}

static void check_triggers(uint8_t flags) {
    char ch;
    if (!(flags & TF_TRIGGER)) return;
    ch = map_char((uint16_t)(UNFIX(hero.x) + 5) >> 3, (uint16_t)(UNFIX(hero.y) + 7) >> 3);
    if (ch == 'S') {
        if (KEY_PRESSED(J_UP)) {
            dialog_show("BONUS RUN\fGrab every coin to open the exit door.\f"
                        "A jumps. Hold A to jump higher. Stomp rats from above!");
            hud_show();
        }
    } else if (ch == 'H' || ch == 'h' || ch == 'J' || ch == 'j') {
        if (door_open) {
            state = ST_DOOR; timer = 0;
            hero.vx = hero.vy = 0;
            anim_play(&hero_anim, &anim_back);
            music_play(&mus_solved);
        } else if (KEY_PRESSED(J_UP)) {
            dialog_show("The door is locked.\nCollect all the coins first!");
            hud_show();
        }
    }
}

static void update_play(void) {
    int16_t target = 0;
    uint8_t ground, flags;

    /* 1. input */
    if (KEY_PRESSED(J_START)) {
        if (dialog_choice("PAUSED", pause_options, 2) == 1) scene_goto(&scene_title, TRANS_FADE_BLACK);
        hud_show();
        return;
    }
    if (KEY_HELD(J_LEFT))       { target = -RUN_SPEED; facing_left = 1; }
    else if (KEY_HELD(J_RIGHT)) { target = RUN_SPEED;  facing_left = 0; }
    hero.vx = approach(hero.vx, target, target ? RUN_ACCEL : RUN_DECEL);

    ground = body_on_ground(&hero);
    if (ground && KEY_PRESSED(J_A)) { hero.vy = JUMP_VY; sfx_play(&sfx_jump); ground = 0; }
    if (KEY_RELEASED(J_A) && hero.vy < 0) hero.vy >>= 1;      /* variable jump height */

    /* 2. physics */
    hero.vy += GRAVITY;
    if (hero.vy > MAX_FALL) hero.vy = MAX_FALL;
    body_move(&hero);

    flags = body_touch_flags(&hero);
    if (flags & TF_HAZARD) hurt();
    else if (body_on_ground(&hero)) { safe_x = hero.x; safe_y = hero.y; }

    if (UNFIX(hero.y) > (int16_t)map_h_px) {    /* fell into the pit: always costs a heart */
        hero.x = safe_x; hero.y = safe_y; hero.vx = hero.vy = 0;
        invuln = 0;
        hurt();
    }
    if (invuln) invuln--;

    update_coins();
    update_rats();
    if (state == ST_PLAY) check_triggers(flags);

    /* 3. animation */
    if (!body_on_ground(&hero)) anim_play(&hero_anim, &anim_jump);
    else if (hero.vx) anim_play(&hero_anim, &anim_run);
    else anim_play(&hero_anim, &anim_idle);
}

static void update_door(void) {
    int16_t tx = FIX((door_tx << 3) + HERO_OX);
    hero.x = approach(hero.x, tx, FIX(1));
    if (++timer == 70) {
        dialog_show("BONUS CLEAR!\nEvery coin found.");
        scene_goto(&scene_title, TRANS_FADE_WHITE);
    }
}

static void update_dead(void) {
    hero.vy += GRAVITY;
    if (hero.vy > MAX_FALL) hero.vy = MAX_FALL;
    hero.y += hero.vy;
    if (++timer == 120) {
        dialog_show("Out of energy...\nTry again!");
        scene_goto(&scene_title, TRANS_FADE_BLACK);
    }
}

static void bonus_update(void) {
    uint8_t flags = facing_left ? SPR_FLIPX : 0;
    coin_t *c;
    rat_t *r;

    if (state == ST_PLAY) update_play();
    else if (state == ST_DOOR) update_door();
    else update_dead();

    anim_update(&hero_anim);
    anim_update(&coin_anim);
    anim_update(&rat_anim);

    /* camera (also applies the shake), then keep the opened door on screen */
    if (state == ST_DEAD) cam_follow(cam_x + 80, cam_y + 72);
    else cam_follow(UNFIX(hero.x) + 5, UNFIX(hero.y) + 7);
    door_refresh();

    /* draw: hero first (on top), then rats, coins, particles */
    if (state == ST_DOOR) flags = SPR_BEHIND;              /* the door frame covers the keeper */
    else if (state == ST_DEAD) flags |= SPR_FLIPY;
    if (!(invuln & 4))
        spr_draw(&spr_hero, anim_frame(&hero_anim),
                 W2S_X(UNFIX(hero.x) - HERO_OX), W2S_Y(UNFIX(hero.y) - HERO_OY), flags);
    for (r = rats; r < rats + MAX_RATS; r++) {
        if (r->state == RAT_OFF) continue;
        flags = r->right ? SPR_FLIPX : 0;
        if (r->state == RAT_DYING) flags |= SPR_FLIPY;
        spr_draw(&spr_r, anim_frame(&rat_anim), W2S_X(UNFIX(r->b.x) - 1), W2S_Y(UNFIX(r->b.y)), flags);
    }
    for (c = coins; c < coins + coin_count; c++)
        if (c->active) spr_draw(&spr_c, anim_frame(&coin_anim), W2S_X(c->x), W2S_Y(c->y), 0);
    particles_update();
}

const scene_t scene_bonus = { bonus_enter, bonus_update, 0 };
