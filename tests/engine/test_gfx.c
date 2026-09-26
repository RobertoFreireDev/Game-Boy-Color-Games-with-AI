/* src/engine/gfx.c: palettes through the fade, sprite VRAM allocator */
#pragma bank 255
#include "unit.h"
#include "data_banked.h"

extern const uint8_t font_main[];   /* used as arbitrary tile data */

static const palette_color_t pal_a[4] = { RGB(1,2,3), RGB(31,0,0), RGB(0,31,0), RGB(0,0,31) };
static const sprite_def_t def_2x2x2 = { font_main, 2, 2, 2, pal_a, 3 };   /* 8 tiles */
static const sprite_def_t def_1x1   = { font_main, 1, 1, 1, pal_a, 1 };
static const sprite_def_t def_128   = { font_main, 4, 4, 8, pal_a, 2 };   /* exactly 128 tiles */
static const sprite_def_t def_129   = { font_main, 1, 1, 129, pal_a, 2 };

static sprite_t s, s2;

void unit_setup(void) BANKED {
    unit_reset_engine();
    fade_set_level(0);
    s.base = s2.base = 0xAA;
}

TEST(bkg_palette_goes_to_ram_and_hardware) {
    uint8_t i;
    gfx_set_bkg_palette(2, pal_a);
    for (i = 0; i < 4; i++) {
        ASSERT_EQ(pal_ram[8 + i], pal_a[i]);
        ASSERT_EQ(hw_bkg_color(2, i), pal_a[i]);
    }
}

TEST(obj_palette_uses_upper_half) {
    uint8_t i;
    gfx_set_obj_palette(5, pal_a);
    for (i = 0; i < 4; i++) {
        ASSERT_EQ(pal_ram[32 + 20 + i], pal_a[i]);
        ASSERT_EQ(hw_obj_color(5, i), pal_a[i]);
    }
}

TEST(palette_respects_fade_level) {
    fade_set_level(8);                      /* fully black */
    gfx_set_bkg_palette(3, pal_a);
    ASSERT_EQ(pal_ram[13], pal_a[1]);
    ASSERT_EQ(hw_bkg_color(3, 1), 0);
    fade_set_level(0);
    ASSERT_EQ(hw_bkg_color(3, 1), pal_a[1]);
}

TEST(load_sprite_fills_handle) {
    ASSERT_EQ(gfx_load_sprite(&s, &def_2x2x2, 0), 1);
    ASSERT_EQ(s.base, 0);
    ASSERT_EQ(s.w, 2);
    ASSERT_EQ(s.h, 2);
    ASSERT_EQ(s.tpf, 4);
    ASSERT_EQ(s.frames, 2);
    ASSERT_EQ(s.pal, 3);
}

TEST(load_sprite_copies_tiles_to_vram_bank0) {
    uint8_t i;
    VBK_REG = 1;
    gfx_load_sprite(&s, &def_2x2x2, 0);
    ASSERT_EQ(VBK_REG & 1, 0);
    for (i = 0; i < 8 * 16; i++) ASSERT_EQ(hw_vram(0, 0x8000 + i), font_main[i]);
}

TEST(load_sprite_sets_obj_palette) {
    uint8_t i;
    gfx_load_sprite(&s, &def_2x2x2, 0);
    for (i = 0; i < 4; i++) {
        ASSERT_EQ(pal_ram[32 + 12 + i], pal_a[i]);
        ASSERT_EQ(hw_obj_color(3, i), pal_a[i]);
    }
}

TEST(sprites_are_allocated_back_to_back) {
    sprite_t s3;
    uint8_t i;
    gfx_load_sprite(&s, &def_2x2x2, 0);
    gfx_load_sprite(&s2, &def_1x1, 0);
    gfx_load_sprite(&s3, &def_2x2x2, 0);
    ASSERT_EQ(s.base, 0);
    ASSERT_EQ(s2.base, 8);
    ASSERT_EQ(s3.base, 9);
    for (i = 0; i < 16; i++) ASSERT_EQ(hw_vram(0, 0x8000 + 9 * 16 + i), font_main[i]);
}

TEST(exactly_128_tiles_fit) {
    ASSERT_EQ(gfx_load_sprite(&s, &def_128, 0), 1);
    ASSERT_EQ(s.base, 0);
    ASSERT_EQ(gfx_load_sprite(&s2, &def_1x1, 0), 0);
    ASSERT_EQ(s2.base, 0xAA);               /* handle untouched on failure */
}

TEST(overflow_fails_without_allocating) {
    ASSERT_EQ(gfx_load_sprite(&s, &def_129, 0), 0);
    ASSERT_EQ(s.base, 0xAA);
    ASSERT_EQ(gfx_load_sprite(&s2, &def_1x1, 0), 1);
    ASSERT_EQ(s2.base, 0);
}

TEST(reset_restarts_allocation) {
    gfx_load_sprite(&s, &def_2x2x2, 0);
    gfx_reset();
    gfx_load_sprite(&s2, &def_1x1, 0);
    ASSERT_EQ(s2.base, 0);
}

TEST(load_banked_sprite_restores_bank) {
    uint8_t save = CURRENT_BANK;
    ASSERT(BANK(spr_banked) != save);        /* really switches away from the test bank */
    ASSERT_EQ(gfx_load_sprite(&s, &spr_banked, BANK(spr_banked)), 1);
    ASSERT_EQ(CURRENT_BANK, save);
    ASSERT_EQ(s.frames, 2);
    ASSERT_EQ(s.pal, 5);
    ASSERT_EQ(hw_vram(0, 0x8000), 0x01);
    ASSERT_EQ(hw_vram(0, 0x800F), 0x10);
    ASSERT_EQ(hw_vram(0, 0x8010), 0xF1);
    ASSERT_EQ(hw_vram(0, 0x801F), 0xEE);
    ASSERT_EQ(hw_obj_color(5, 1), RGB(3, 5, 7));
    ASSERT_EQ(hw_obj_color(5, 3), RGB(15, 17, 19));
}

void unit_tests(void) BANKED {
    RUN(bkg_palette_goes_to_ram_and_hardware);
    RUN(obj_palette_uses_upper_half);
    RUN(palette_respects_fade_level);
    RUN(load_sprite_fills_handle);
    RUN(load_sprite_copies_tiles_to_vram_bank0);
    RUN(load_sprite_sets_obj_palette);
    RUN(sprites_are_allocated_back_to_back);
    RUN(exactly_128_tiles_fit);
    RUN(overflow_fails_without_allocating);
    RUN(reset_restarts_allocation);
    RUN(load_banked_sprite_restores_bank);
}
