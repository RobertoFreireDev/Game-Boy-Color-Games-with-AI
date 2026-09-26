/* src/engine/scene.c: scene_start, scene_goto + transitions, scene_update, scene_reset_screen */
#pragma bank 255
#include "unit.h"

extern const uint8_t font_main[];

static const palette_color_t pal[4] = { RGB(0,0,0), RGB(20,10,5), RGB(5,10,20), RGB(31,31,31) };
static const sprite_def_t def_1x1 = { font_main, 1, 1, 1, pal, 0 };
static sprite_t spr;
static const particle_style_t puff = { &spr, 0, 0, 0, 50 };
static int16_t tw_value;

static uint8_t a_enter, a_update, a_leave, b_enter, b_update, a_draw;
static const scene_t *goto_next;
static uint8_t goto_trans;
static uint16_t leave_color;
static uint8_t b_saw_press, b_updates_at_enter;

static void a_enter_fn(void) { a_enter++; gfx_set_bkg_palette(0, pal); }
static void a_update_fn(void) {
    a_update++;
    if (a_draw) spr_put(1, 10, 10, 0);
    if (goto_next) { scene_goto(goto_next, goto_trans); goto_next = 0; }
}
static void a_leave_fn(void) { a_leave++; leave_color = hw_bkg_color(0, 1); }
static void b_enter_fn(void) { b_enter++; b_updates_at_enter = b_update; }
static void b_update_fn(void) { b_update++; if (KEY_PRESSED(J_START)) b_saw_press = 1; }

static const scene_t scene_a = { a_enter_fn, a_update_fn, a_leave_fn };
static const scene_t scene_b = { b_enter_fn, b_update_fn, 0 };

void unit_setup(void) BANKED {
    unit_reset_engine();
    fade_init();
    a_enter = a_update = a_leave = b_enter = b_update = 0;
    a_draw = 0;
    goto_next = 0;
    b_saw_press = 0;
    leave_color = 0xFFFF;
}

TEST(start_enters_draws_one_frame_and_fades_in) {
    uint16_t t0 = sys_time;
    a_draw = 1;
    scene_start(&scene_a);
    ASSERT_EQ(a_enter, 1);
    ASSERT_EQ(a_update, 1);
    ASSERT_EQ(shadow_OAM[0].tile, 1);
    ASSERT_EQ(shadow_OAM[0].y, 26);
    ASSERT_EQ(hw_bkg_color(0, 1), pal[1]);
    ASSERT((uint16_t)(sys_time - t0) >= 32);  /* fade_in(4): 8 steps x 4 frames */
}

TEST(update_runs_current_scene_only) {
    scene_start(&scene_a);
    scene_update();
    scene_update();
    ASSERT_EQ(a_update, 3);
    ASSERT_EQ(a_leave, 0);
    ASSERT_EQ(b_enter, 0);
}

TEST(update_hides_sprites_not_redrawn) {
    a_draw = 1;
    scene_start(&scene_a);
    ASSERT(shadow_OAM[0].y != 0);
    a_draw = 0;
    scene_update();
    ASSERT_EQ(shadow_OAM[0].y, 0);
}

TEST(goto_waits_for_end_of_frame) {
    scene_start(&scene_a);
    scene_goto(&scene_b, TRANS_NONE);
    ASSERT_EQ(a_leave, 0);                 /* nothing happens until scene_update */
    ASSERT_EQ(b_enter, 0);
    scene_update();
    ASSERT_EQ(a_update, 2);                /* the current frame still ran */
    ASSERT_EQ(a_leave, 1);
    ASSERT_EQ(b_enter, 1);
}

TEST(goto_none_switches_in_one_update) {
    uint16_t t0;
    scene_start(&scene_a);
    goto_next = &scene_b; goto_trans = TRANS_NONE;
    t0 = sys_time;
    scene_update();
    ASSERT((uint16_t)(sys_time - t0) < 8);    /* no fade (a fade takes 32 frames) */
    ASSERT_EQ(a_update, 2);
    ASSERT_EQ(a_leave, 1);
    ASSERT_EQ(leave_color, pal[1]);        /* no fade: still visible in leave() */
    ASSERT_EQ(b_enter, 1);
    ASSERT_EQ(b_updates_at_enter, 0);
    ASSERT_EQ(b_update, 1);                /* one frame drawn right after enter */
    ASSERT_EQ(hw_bkg_color(0, 1), pal[1]);
    scene_update();
    ASSERT_EQ(b_update, 2);
    ASSERT_EQ(a_update, 2);
}

TEST(goto_black_fades_out_and_in) {
    scene_start(&scene_a);
    goto_next = &scene_b; goto_trans = TRANS_FADE_BLACK;
    scene_update();
    ASSERT_EQ(leave_color, 0);
    ASSERT_EQ(b_enter, 1);
    ASSERT_EQ(hw_bkg_color(0, 1), pal[1]);
}

TEST(goto_white_fades_out_and_in) {
    scene_start(&scene_a);
    goto_next = &scene_b; goto_trans = TRANS_FADE_WHITE;
    scene_update();
    ASSERT_EQ(leave_color, RGB(31, 31, 31));
    ASSERT_EQ(hw_bkg_color(0, 1), pal[1]);
}

TEST(goto_resets_engine_state) {
    sprite_t s2;
    scene_start(&scene_a);
    gfx_load_sprite(&spr, &def_1x1, 0);
    gfx_load_sprite(&s2, &def_1x1, 0);
    ASSERT_EQ(s2.base, 1);
    tw_value = 0;
    tween_start(&tw_value, 100, 50, EASE_LINEAR);
    particles_emit(40, 40, 3, &puff);
    cam_x = 40; cam_y = 24;
    set_bkg_tile_xy(3, 3, 0x42);
    SHOW_WIN;
    move_bkg(5, 6);
    goto_next = &scene_b; goto_trans = TRANS_NONE;
    scene_update();
    ASSERT(!tween_busy(&tw_value));
    ASSERT_EQ(cam_x, 0);
    ASSERT_EQ(cam_y, 0);
    ASSERT_EQ(hw_bkg_tile(3, 3), TILE_BLANK);
    ASSERT(!(LCDC_REG & LCDCF_WINON));
    ASSERT_EQ(SCX_REG, 0);
    ASSERT_EQ(SCY_REG, 0);
    particles_update();                    /* particle pool was cleared: nothing drawn */
    ASSERT_EQ(shadow_OAM[0].y, 0);
    gfx_load_sprite(&s2, &def_1x1, 0);
    ASSERT_EQ(s2.base, 0);                 /* sprite VRAM allocator restarted */
}

TEST(goto_eats_the_press_that_caused_it) {
    static const uint8_t s[] = { 0, J_START, J_START };
    scene_start(&scene_a);
    pad_script(s, 3);
    input_update();
    input_update();                        /* START pressed this frame */
    ASSERT(KEY_PRESSED(J_START));
    goto_next = &scene_b; goto_trans = TRANS_NONE;
    scene_update();
    ASSERT_EQ(b_saw_press, 0);
    ASSERT_EQ(keys_prev, J_START);
}

TEST(scene_without_leave_callback) {
    scene_start(&scene_b);
    scene_goto(&scene_a, TRANS_NONE);
    scene_update();
    ASSERT_EQ(a_enter, 1);
    ASSERT_EQ(a_update, 1);
}

TEST(reset_screen_clears_everything) {
    VBK_REG = 1;
    set_bkg_tile_xy(4, 4, 0x03);
    set_win_tile_xy(2, 2, 0x03);
    VBK_REG = 0;
    set_bkg_tile_xy(31, 31, 0x42);
    set_win_tile_xy(2, 2, 0x42);
    SHOW_WIN;
    move_bkg(9, 9);
    spr_begin();
    spr_put(1, 20, 20, 0);
    scene_reset_screen();
    ASSERT_EQ(hw_bkg_attr(4, 4), ATTR_BLANK);
    ASSERT_EQ(hw_bkg_tile(31, 31), TILE_BLANK);
    ASSERT_EQ(hw_win_tile(2, 2), TILE_BLANK);
    ASSERT_EQ(hw_win_attr(2, 2), ATTR_BLANK);
    ASSERT(!(LCDC_REG & LCDCF_WINON));
    ASSERT_EQ(SCX_REG, 0);
    ASSERT_EQ(shadow_OAM[0].y, 0);
    ASSERT_EQ(VBK_REG & 1, 0);
}

void unit_tests(void) BANKED {
    RUN(start_enters_draws_one_frame_and_fades_in);
    RUN(update_runs_current_scene_only);
    RUN(update_hides_sprites_not_redrawn);
    RUN(goto_waits_for_end_of_frame);
    RUN(goto_none_switches_in_one_update);
    RUN(goto_black_fades_out_and_in);
    RUN(goto_white_fades_out_and_in);
    RUN(goto_resets_engine_state);
    RUN(goto_eats_the_press_that_caused_it);
    RUN(scene_without_leave_callback);
    RUN(reset_screen_clears_everything);
}
