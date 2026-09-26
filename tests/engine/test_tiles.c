/* src/engine/tiles.h: PX() pixel rows -> Game Boy 2bpp bytes */
#pragma bank 255
#include "unit.h"

static const uint8_t row_mixed[] = { PX(0,1,2,3,0,1,2,3) };
static const uint8_t row_all3[]  = { PX(3,3,3,3,3,3,3,3) };
static const uint8_t row_zero[]  = { PX(0,0,0,0,0,0,0,0) };
static const uint8_t tile[] = {
    PX(1,0,0,0,0,0,0,0), PX(0,2,0,0,0,0,0,0), PX(0,0,3,0,0,0,0,0), PX(0,0,0,1,0,0,0,0),
    PX(0,0,0,0,2,0,0,0), PX(0,0,0,0,0,3,0,0), PX(0,0,0,0,0,0,1,0), PX(0,0,0,0,0,0,0,2),
};

void unit_setup(void) BANKED { }

TEST(px_is_two_bytes) {
    ASSERT_EQ(sizeof(row_mixed), 2);
    ASSERT_EQ(sizeof(tile), TILE_BYTES);
    ASSERT_EQ(TILE_BYTES, 16);
}

TEST(px_low_plane_first) {
    ASSERT_EQ(row_mixed[0], 0x55);   /* bit 0 of each pixel: 0 1 0 1 0 1 0 1 */
    ASSERT_EQ(row_mixed[1], 0x33);   /* bit 1 of each pixel: 0 0 1 1 0 0 1 1 */
}

TEST(px_extremes) {
    ASSERT_EQ(row_all3[0], 0xFF);
    ASSERT_EQ(row_all3[1], 0xFF);
    ASSERT_EQ(row_zero[0], 0x00);
    ASSERT_EQ(row_zero[1], 0x00);
}

TEST(px_leftmost_pixel_is_bit7) {
    ASSERT_EQ(tile[0], 0x80);  ASSERT_EQ(tile[1], 0x00);   /* 1 at x0 */
    ASSERT_EQ(tile[2], 0x00);  ASSERT_EQ(tile[3], 0x40);   /* 2 at x1 */
    ASSERT_EQ(tile[4], 0x20);  ASSERT_EQ(tile[5], 0x20);   /* 3 at x2 */
    ASSERT_EQ(tile[14], 0x00); ASSERT_EQ(tile[15], 0x01);  /* 2 at x7 */
}

TEST(px_macro_ignores_high_bits) {
    uint8_t lo = PXL_(5,0,0,0,0,0,0,0), hi = PXH_(6,0,0,0,0,0,0,0);
    ASSERT_EQ(lo, 0x80);   /* 5 & 1 = 1 */
    ASSERT_EQ(hi, 0x80);   /* (6 >> 1) & 1 = 1 */
}

void unit_tests(void) BANKED {
    RUN(px_is_two_bytes);
    RUN(px_low_plane_first);
    RUN(px_extremes);
    RUN(px_leftmost_pixel_is_bit7);
    RUN(px_macro_ignores_high_bits);
}
