/* src/engine/sprites.c: OAM allocator, culling, metasprite layout and flips */
#pragma bank 255
#include "unit.h"

/* 2x2 sprite at tile 10, 3 frames, OBJ palette 2 */
static const sprite_t s22 = { 10, 2, 2, 4, 3, 2 };

void unit_setup(void) BANKED {
    unit_reset_engine();
    spr_begin();
}

static uint8_t oam_tile(uint8_t i) { return shadow_OAM[i].tile; }

TEST(put_writes_oam_with_hardware_offsets) {
    ASSERT_EQ(spr_put(7, 10, 20, 0x23), 1);
    ASSERT_EQ(shadow_OAM[0].x, 18);
    ASSERT_EQ(shadow_OAM[0].y, 36);
    ASSERT_EQ(shadow_OAM[0].tile, 7);
    ASSERT_EQ(shadow_OAM[0].prop, 0x23);
}

TEST(put_uses_consecutive_slots) {
    spr_put(1, 0, 0, 0);
    spr_put(2, 8, 0, 0);
    spr_put(3, 16, 0, 0);
    ASSERT_EQ(oam_tile(0), 1);
    ASSERT_EQ(oam_tile(1), 2);
    ASSERT_EQ(oam_tile(2), 3);
}

TEST(put_culls_fully_offscreen) {
    ASSERT_EQ(spr_put(1, -8, 0, 0), 0);
    ASSERT_EQ(spr_put(1, 160, 0, 0), 0);
    ASSERT_EQ(spr_put(1, 0, -8, 0), 0);
    ASSERT_EQ(spr_put(1, 0, 144, 0), 0);
    ASSERT_EQ(spr_put(1, -1000, 5000, 0), 0);
    ASSERT_EQ(spr_put(2, -7, 0, 0), 1);   ASSERT_EQ(shadow_OAM[0].x, 1);
    ASSERT_EQ(spr_put(3, 159, 0, 0), 1);  ASSERT_EQ(shadow_OAM[1].x, 167);
    ASSERT_EQ(spr_put(4, 0, -7, 0), 1);   ASSERT_EQ(shadow_OAM[2].y, 9);
    ASSERT_EQ(spr_put(5, 0, 143, 0), 1);  ASSERT_EQ(shadow_OAM[3].y, 159);
    ASSERT_EQ(oam_tile(0), 2);            /* culled calls used no slot */
}

TEST(put_stops_at_40) {
    uint8_t i;
    for (i = 0; i < 40; i++) ASSERT_EQ(spr_put(i, i, 0, 0), 1);
    ASSERT_EQ(spr_put(99, 0, 0, 0), 0);
    ASSERT_EQ(oam_tile(39), 39);
}

TEST(end_hides_slots_not_used_this_frame) {
    spr_put(1, 0, 0, 0); spr_put(2, 0, 0, 0); spr_put(3, 0, 0, 0);
    spr_end();
    spr_begin();
    spr_put(4, 0, 0, 0);
    spr_end();
    ASSERT(shadow_OAM[0].y != 0);
    ASSERT_EQ(oam_tile(0), 4);
    ASSERT_EQ(shadow_OAM[1].y, 0);
    ASSERT_EQ(shadow_OAM[2].y, 0);
    spr_begin();
    spr_put(5, 0, 0, 0); spr_put(6, 0, 0, 0);
    spr_end();
    ASSERT(shadow_OAM[1].y != 0);
    ASSERT_EQ(shadow_OAM[2].y, 0);
}

TEST(oam_reaches_hardware_after_vsync) {
    spr_put(42, 30, 40, 0x05);
    vsync();                             /* VBlank handler DMAs shadow OAM */
    ASSERT_EQ(*(volatile uint8_t *)0xFE00, 56);
    ASSERT_EQ(*(volatile uint8_t *)0xFE01, 38);
    ASSERT_EQ(*(volatile uint8_t *)0xFE02, 42);
    ASSERT_EQ(*(volatile uint8_t *)0xFE03, 0x05);
}

TEST(draw_lays_out_rows_and_columns) {
    spr_draw(&s22, 1, 50, 40, 0);        /* frame 1 = tiles 14..17 */
    ASSERT_EQ(oam_tile(0), 14); ASSERT_EQ(shadow_OAM[0].x, 58); ASSERT_EQ(shadow_OAM[0].y, 56);
    ASSERT_EQ(oam_tile(1), 15); ASSERT_EQ(shadow_OAM[1].x, 66); ASSERT_EQ(shadow_OAM[1].y, 56);
    ASSERT_EQ(oam_tile(2), 16); ASSERT_EQ(shadow_OAM[2].x, 58); ASSERT_EQ(shadow_OAM[2].y, 64);
    ASSERT_EQ(oam_tile(3), 17); ASSERT_EQ(shadow_OAM[3].x, 66); ASSERT_EQ(shadow_OAM[3].y, 64);
    ASSERT_EQ(shadow_OAM[0].prop, 2);
    ASSERT_EQ(shadow_OAM[3].prop, 2);
}

TEST(draw_flip_x_swaps_columns) {
    spr_draw(&s22, 0, 50, 40, SPR_FLIPX);
    ASSERT_EQ(oam_tile(0), 11); ASSERT_EQ(shadow_OAM[0].x, 58);
    ASSERT_EQ(oam_tile(1), 10);
    ASSERT_EQ(oam_tile(2), 13);
    ASSERT_EQ(oam_tile(3), 12);
    ASSERT_EQ(shadow_OAM[0].prop, SPR_FLIPX | 2);
}

TEST(draw_flip_y_swaps_rows) {
    spr_draw(&s22, 0, 50, 40, SPR_FLIPY);
    ASSERT_EQ(oam_tile(0), 12); ASSERT_EQ(shadow_OAM[0].y, 56);
    ASSERT_EQ(oam_tile(1), 13);
    ASSERT_EQ(oam_tile(2), 10);
    ASSERT_EQ(oam_tile(3), 11);
    ASSERT_EQ(shadow_OAM[0].prop, SPR_FLIPY | 2);
}

TEST(draw_both_flips_and_behind) {
    spr_draw(&s22, 2, 0, 0, SPR_FLIPX | SPR_FLIPY | SPR_BEHIND);   /* frame 2 = 18..21 */
    ASSERT_EQ(oam_tile(0), 21);
    ASSERT_EQ(oam_tile(1), 20);
    ASSERT_EQ(oam_tile(2), 19);
    ASSERT_EQ(oam_tile(3), 18);
    ASSERT_EQ(shadow_OAM[0].prop, 0xE2);
}

TEST(draw_culls_offscreen_tiles_only) {
    spr_draw(&s22, 0, -8, 40, 0);        /* left column is off screen */
    spr_end();
    ASSERT_EQ(oam_tile(0), 11); ASSERT_EQ(shadow_OAM[0].x, 8);
    ASSERT_EQ(oam_tile(1), 13);
    ASSERT_EQ(shadow_OAM[2].y, 0);
}

TEST(hide_all_clears_and_restarts) {
    uint8_t i;
    for (i = 0; i < 5; i++) spr_put(i, 0, 0, 0);
    spr_hide_all();
    for (i = 0; i < 40; i++) ASSERT_EQ(shadow_OAM[i].y, 0);
    spr_begin();
    spr_put(9, 0, 0, 0);
    ASSERT_EQ(oam_tile(0), 9);
}

void unit_tests(void) BANKED {
    RUN(put_writes_oam_with_hardware_offsets);
    RUN(put_uses_consecutive_slots);
    RUN(put_culls_fully_offscreen);
    RUN(put_stops_at_40);
    RUN(end_hides_slots_not_used_this_frame);
    RUN(oam_reaches_hardware_after_vsync);
    RUN(draw_lays_out_rows_and_columns);
    RUN(draw_flip_x_swaps_columns);
    RUN(draw_flip_y_swaps_rows);
    RUN(draw_both_flips_and_behind);
    RUN(draw_culls_offscreen_tiles_only);
    RUN(hide_all_clears_and_restarts);
}
