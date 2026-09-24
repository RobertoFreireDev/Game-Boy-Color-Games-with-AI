#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"
#include "game/board.h"
#include "game/state.h"

#define ST_INTRO 0
#define ST_PLAY  1
#define ST_WON   2

#define STEP_FRAMES 8          /* one cell (16 px) in 8 frames = 2 px per frame */

static sprite_t spr_hero, spr_box;
static anim_t hero_anim;
static uint8_t state, timer;
static uint8_t facing;
static uint8_t moving;                 /* frames left in the current step */
static uint8_t box_moving, box_dest;
static int16_t hero_x, hero_y, box_x, box_y;   /* world pixels */
static uint16_t moves, pushes;
static uint8_t help_shown;             /* the help dialog appears once per power-on */

static const uint8_t dir_keys[4] = { J_UP, J_DOWN, J_LEFT, J_RIGHT };
static const int8_t dir_dx[4] = { 0, 0, -2, 2 };
static const int8_t dir_dy[4] = { -2, 2, 0, 0 };

/* player frames: 0/1 down, 2/3 up, 4/5 right (left = flipped) */
static const uint8_t fr_up[] = { 2 }, fr_down[] = { 0 }, fr_side[] = { 4 };
static const uint8_t fr_up_walk[] = { 3, 2 }, fr_down_walk[] = { 1, 0 }, fr_side_walk[] = { 5, 4 };
static const anim_def_t anim_idle[4] = {
    { fr_up, 1, 1, 1 }, { fr_down, 1, 1, 1 }, { fr_side, 1, 1, 1 }, { fr_side, 1, 1, 1 } };
static const anim_def_t anim_walk[4] = {
    { fr_up_walk, 2, 4, 1 }, { fr_down_walk, 2, 4, 1 }, { fr_side_walk, 2, 4, 1 }, { fr_side_walk, 2, 4, 1 } };

static const char * const pause_options[] = { "CONTINUE", "RESTART", "QUIT TO TITLE" };

static void hud_update(void) {
    text_print_num_win(11, 0, MIN(moves, 999), 3);
    text_print_num_win(17, 0, MIN(pushes, 999), 3);
}

static void hud_show(void) {
    text_print_win(0, 0, "LEVEL 00 M 000 P 000");
    text_print_num_win(6, 0, game_level + 1, 2);
    hud_update();
    move_win(7, 136);
    SHOW_WIN;
}

static void hero_snap(void) {
    hero_x = cell_px_x(player_cell);
    hero_y = cell_px_y(player_cell);
}

static void level_enter(void) {
    board_load(game_level);
    cam_set(0, 0);
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    gfx_load_sprite(&spr_box, &spr_crate, 0);
    hero_snap();
    facing = DIR_DOWN;
    moving = 0; box_moving = 0;
    moves = 0; pushes = 0;
    hero_anim.def = 0;
    anim_play(&hero_anim, &anim_idle[DIR_DOWN]);
    hud_show();
    state = ST_INTRO; timer = 0;
    music_play(&mus_puzzle);
}

static void draw_sprites(void) {
    spr_draw(&spr_hero, anim_frame(&hero_anim), W2S_X(hero_x), W2S_Y(hero_y),
             facing == DIR_LEFT ? SPR_FLIPX : 0);
    if (box_moving) spr_draw(&spr_box, 0, W2S_X(box_x), W2S_Y(box_y), 0);
}

static void start_step(uint8_t dir) {
    uint8_t r, from;
    facing = dir;
    r = board_try_move(dir);
    if (r == MOVE_BLOCKED) {
        if (KEY_PRESSED(dir_keys[dir])) sfx_play(&sfx_bump);
        return;
    }
    moving = STEP_FRAMES;
    moves++;
    if (r == MOVE_PUSH) {
        pushes++;
        from = player_cell;                       /* the crate was where the player now goes */
        box_dest = from + dir_off[dir];
        box_x = cell_px_x(from); box_y = cell_px_y(from);
        box_moving = 1;
        board_draw_cell(from);                    /* BG shows floor/goal; the sprite slides instead */
        sfx_play(&sfx_push);
    } else {
        sfx_play(&sfx_step);
    }
    hud_update();
}

static void do_undo(void) {
    uint8_t r = board_undo();
    if (r == UNDO_NONE) { sfx_play(&sfx_bump); return; }
    facing = r & 3;
    moves--;
    if (r & UNDO_PUSHED) pushes--;
    hero_snap();
    sfx_play(&sfx_undo);
    hud_update();
}

static void pause_menu(void) {
    uint8_t choice = dialog_choice("PAUSED", pause_options, 3);
    if (choice == 1) scene_goto(&scene_level, TRANS_FADE_BLACK);
    else if (choice == 2) scene_goto(&scene_title, TRANS_FADE_BLACK);
    else hud_show();
}

static void finish_step(void) {
    if (box_moving) {
        box_moving = 0;
        board_draw_cell(box_dest);
        if (board[box_dest] & CELL_GOAL) sfx_play(&sfx_goal);
    }
    if (board_solved()) {
        state = ST_WON; timer = 0;
        game_moves = moves; game_pushes = pushes;
        game_record_solve();
        music_play(&mus_solved);
    }
}

static void level_update(void) {
    uint8_t d;

    if (state == ST_INTRO) {
        draw_sprites();
        /* frame 0 runs before the fade-in; show the help once the screen is visible */
        if (help_shown) { state = ST_PLAY; return; }
        if (++timer >= 2) {
            help_shown = 1;
            dialog_show("Push every crate onto a yellow mark.\f"
                        "D-PAD move  B undo\nSELECT restart\nSTART pause");
            hud_show();
            state = ST_PLAY;
        }
        return;
    }

    if (state == ST_WON) {
        anim_play(&hero_anim, &anim_idle[DIR_DOWN]);
        facing = DIR_DOWN;
        draw_sprites();
        if (++timer == 90) {
            dialog_show(game_level + 1 < LEVEL_COUNT ? "All crates are in place. Great work!"
                                                     : "The whole warehouse is in order!");
            scene_goto(&scene_solved, TRANS_FADE_WHITE);
        }
        return;
    }

    /* 1. input (only between steps) */
    if (!moving) {
        if (KEY_PRESSED(J_START)) { draw_sprites(); pause_menu(); return; }
        if (KEY_PRESSED(J_SELECT)) { scene_goto(&scene_level, TRANS_FADE_BLACK); }
        else if (KEY_PRESSED(J_B)) do_undo();
        else {
            for (d = 0; d < 4; d++) if (KEY_HELD(dir_keys[d])) { start_step(d); break; }
        }
    }

    /* 2. movement */
    if (moving) {
        hero_x += dir_dx[facing]; hero_y += dir_dy[facing];
        if (box_moving) { box_x += dir_dx[facing]; box_y += dir_dy[facing]; }
        anim_play(&hero_anim, &anim_walk[facing]);
        if (--moving == 0) finish_step();
    } else {
        anim_play(&hero_anim, &anim_idle[facing]);
    }
    anim_update(&hero_anim);

    /* 3. draw */
    draw_sprites();
}

const scene_t scene_level = { level_enter, level_update, 0 };
