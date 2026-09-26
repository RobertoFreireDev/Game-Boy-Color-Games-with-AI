/* src/engine/fade.c: palette RAM, fade levels to black/white, blocking fades */
#pragma bank 255
#include "unit.h"

#define C1 RGB(31, 16, 8)

void unit_setup(void) BANKED {
    unit_reset_engine();
    fade_init();
    pal_ram[6] = C1;                       /* BG palette 1, color 2 */
    pal_ram[32 + 27] = C1;                 /* OBJ palette 6, color 3 */
}

static uint16_t frames_of(void (*f)(uint8_t, uint8_t), uint8_t fps, uint8_t white) {
    uint16_t t0;
    vsync();
    t0 = sys_time;
    f(fps, white);
    return sys_time - t0;
}

TEST(init_clears_ram_and_starts_black) {
    uint8_t i;
    fade_init();
    for (i = 0; i < 64; i++) ASSERT_EQ(pal_ram[i], 0);
    pal_ram[6] = C1;
    fade_apply();
    ASSERT_EQ(hw_bkg_color(1, 2), 0);      /* level 8 = fully black */
}

TEST(level_0_shows_pal_ram) {
    fade_set_level(0);
    ASSERT_EQ(hw_bkg_color(1, 2), C1);
    ASSERT_EQ(hw_obj_color(6, 3), C1);
    ASSERT_EQ(hw_bkg_color(0, 0), 0);
}

TEST(all_64_colors_reach_hardware) {
    uint8_t i;
    for (i = 0; i < 64; i++) pal_ram[i] = RGB(i & 31, 31 - (i & 31), (i >> 1) & 31);
    fade_set_level(0);
    for (i = 0; i < 32; i++) {
        ASSERT_EQ(hw_bkg_color(i >> 2, i & 3), pal_ram[i]);
        ASSERT_EQ(hw_obj_color(i >> 2, i & 3), pal_ram[32 + i]);
    }
}

TEST(black_levels_scale_channels) {
    fade_set_level(4);
    ASSERT_EQ(hw_bkg_color(1, 2), RGB(16, 8, 4));
    ASSERT_EQ(hw_obj_color(6, 3), RGB(16, 8, 4));
    fade_set_level(1);
    ASSERT_EQ(hw_bkg_color(1, 2), RGB(28, 14, 7));
    fade_set_level(8);
    ASSERT_EQ(hw_bkg_color(1, 2), 0);
}

TEST(apply_leaves_pal_ram_alone) {
    fade_set_level(5);
    ASSERT_EQ(pal_ram[6], C1);
}

TEST(fade_out_black_takes_8_steps) {
    uint16_t n;
    fade_set_level(0);
    n = frames_of(fade_out, 2, 0);
    ASSERT_EQ(n, 16);
    ASSERT_EQ(hw_bkg_color(1, 2), 0);
}

TEST(fade_out_white_ends_white) {
    fade_set_level(0);
    fade_out(1, 1);
    ASSERT_EQ(hw_bkg_color(1, 2), RGB(31, 31, 31));
    ASSERT_EQ(hw_bkg_color(0, 0), RGB(31, 31, 31));   /* black turns white too */
    ASSERT_EQ(hw_obj_color(6, 3), RGB(31, 31, 31));
}

TEST(white_levels_scale_towards_31) {
    fade_out(1, 1);                        /* selects white mode */
    fade_set_level(4);
    ASSERT_EQ(hw_bkg_color(1, 2), RGB(31, 23, 19));
    ASSERT_EQ(hw_bkg_color(0, 0), RGB(15, 15, 15));
}

TEST(fade_in_from_black) {
    uint16_t n;
    fade_out(1, 0);
    n = frames_of(fade_in, 1, 0);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(hw_bkg_color(1, 2), C1);
    ASSERT_EQ(hw_obj_color(6, 3), C1);
}

TEST(fade_in_from_white_keeps_white_mode) {
    fade_in(1, 1);
    ASSERT_EQ(hw_bkg_color(1, 2), C1);
    fade_set_level(8);
    ASSERT_EQ(hw_bkg_color(1, 2), RGB(31, 31, 31));
}

void unit_tests(void) BANKED {
    RUN(init_clears_ram_and_starts_black);
    RUN(level_0_shows_pal_ram);
    RUN(all_64_colors_reach_hardware);
    RUN(black_levels_scale_channels);
    RUN(apply_leaves_pal_ram_alone);
    RUN(fade_out_black_takes_8_steps);
    RUN(fade_out_white_ends_white);
    RUN(white_levels_scale_towards_31);
    RUN(fade_in_from_black);
    RUN(fade_in_from_white_keeps_white_mode);
}
