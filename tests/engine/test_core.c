/* src/engine/core.c + core.h: engine_init, fixed point, math macros, approach, rand_seed */
#pragma bank 255
#include "unit.h"

extern const uint8_t font_main[];

void unit_setup(void) BANKED { }

/* must run first: checks the state main() left right after engine_init() */
TEST(engine_init_hardware_state) {
    ASSERT_EQ(_cpu, CGB_TYPE);
    ASSERT(KEY1_REG & 0x80);                   /* double speed */
    ASSERT(LCDC_REG & LCDCF_ON);
    ASSERT(LCDC_REG & LCDCF_BGON);
    ASSERT(LCDC_REG & LCDCF_OBJON);
    ASSERT(!(LCDC_REG & LCDCF_WINON));
    ASSERT(!(LCDC_REG & LCDCF_OBJ16));         /* 8x8 sprites */
}

TEST(engine_init_screen_black_and_cleared) {
    uint8_t i, x, y;
    for (i = 0; i < 8; i++) {
        ASSERT_EQ(hw_bkg_color(i, 0), 0); ASSERT_EQ(hw_bkg_color(i, 3), 0);
        ASSERT_EQ(hw_obj_color(i, 1), 0); ASSERT_EQ(hw_obj_color(i, 3), 0);
    }
    for (y = 0; y < 32; y += 7)
        for (x = 0; x < 32; x += 5) {
            ASSERT_EQ(hw_bkg_tile(x, y), TILE_BLANK);
            ASSERT_EQ(hw_bkg_attr(x, y), ATTR_BLANK);
        }
    for (i = 0; i < 40; i++) ASSERT_EQ(shadow_OAM[i].y, 0);
}

TEST(engine_init_font_and_ui_palette) {
    /* 'A' = font tile 128 + 33 in VRAM bank 1; glyph row 0 in both bit planes */
    uint16_t a = 0x8000 + ((uint16_t)(128 + 'A' - 32) << 4);
    ASSERT_EQ(hw_vram(1, a), font_main[('A' - 32) * 8]);
    ASSERT_EQ(hw_vram(1, a + 1), font_main[('A' - 32) * 8]);
    ASSERT_EQ(pal_ram[28], pal_ui_default[0]);
    ASSERT_EQ(pal_ram[31], pal_ui_default[3]);
}

TEST(engine_init_sound_on) {
    ASSERT(NR52_REG & 0x80);
    ASSERT_EQ(NR50_REG, 0x77);
    ASSERT_EQ(NR51_REG, 0xFF);
}

TEST(fix_unfix) {
    ASSERT_EQ(FIX(1), 16);
    ASSERT_EQ(FIX(-3), -48);
    ASSERT_EQ(FIX(2047), 32752);
    ASSERT_EQ(UNFIX(FIX(100)), 100);
    ASSERT_EQ(UNFIX(FIX(-100)), -100);
    ASSERT_EQ(UNFIX(24), 1);           /* 1.5 px truncates down */
    ASSERT_EQ(UNFIX(-1), -1);          /* arithmetic shift: floor */
    ASSERT_EQ(UNFIX(-17), -2);
}

TEST(math_macros) {
    int16_t n = -5;
    ASSERT_EQ(ABS(n), 5);
    ASSERT_EQ(ABS(7), 7);
    ASSERT_EQ(MIN(3, -2), -2);
    ASSERT_EQ(MAX(3, -2), 3);
    ASSERT_EQ(CLAMP(5, 0, 10), 5);
    ASSERT_EQ(CLAMP(-1, 0, 10), 0);
    ASSERT_EQ(CLAMP(11, 0, 10), 10);
}

TEST(approach_moves_by_step) {
    ASSERT_EQ(approach(0, 10, 3), 3);
    ASSERT_EQ(approach(10, 0, 3), 7);
    ASSERT_EQ(approach(-5, -20, 4), -9);
}

TEST(approach_never_overshoots) {
    ASSERT_EQ(approach(8, 10, 3), 10);
    ASSERT_EQ(approach(12, 10, 3), 10);
    ASSERT_EQ(approach(10, 10, 3), 10);
    ASSERT_EQ(approach(0, 0, 0), 0);
}

TEST(rand_seed_uses_div_and_sys_time) {
    uint8_t a, b;
    disable_interrupts();
    sys_time = 0x0012;
    DIV_REG = 0;                  /* any write resets DIV to 0 */
    rand_seed();                  /* read long before DIV ticks again */
    a = rand();
    initrand(0x1200);
    b = rand();
    enable_interrupts();
    ASSERT_EQ(a, b);
}

void unit_tests(void) BANKED {
    RUN(engine_init_hardware_state);
    RUN(engine_init_screen_black_and_cleared);
    RUN(engine_init_font_and_ui_palette);
    RUN(engine_init_sound_on);
    RUN(fix_unfix);
    RUN(math_macros);
    RUN(approach_moves_by_step);
    RUN(approach_never_overshoots);
    RUN(rand_seed_uses_div_and_sys_time);
}
