/* src/engine/map.c: map_load, tile queries, map_find, map_set_tile, camera + streaming */
#pragma bank 255
#include "unit.h"
#include "data_banked.h"

extern const uint8_t font_main[];   /* used as arbitrary tile data */

static const palette_color_t pal0[4] = { RGB(1,1,1), RGB(2,2,2), RGB(3,3,3), RGB(4,4,4) };
static const palette_color_t pal1[4] = { RGB(5,5,5), RGB(6,6,6), RGB(7,7,7), RGB(8,8,8) };
static const palette_color_t pal2[4] = { RGB(9,9,9), RGB(10,10,10), RGB(11,11,11), RGB(12,12,12) };
static const palette_color_t pal3[4] = { RGB(13,0,0), RGB(0,14,0), RGB(0,0,15), RGB(16,16,16) };
static const palette_color_t * const pals[4] = { pal0, pal1, pal2, pal3 };
static const tileset_def_t ts = { font_main, 6, pals, 4 };

/* ---- 20x18 room ---- */
static const map_legend_t legend[] = {
    { '.', 0, 0, 0 },
    { '#', 1, 1, TF_SOLID },
    { 'F', 2, 2, TF_FLIPX | TF_FLIPY | TF_OVER },
    { 'P', 3, 0, TF_SPAWN },
    { 'x', 5, 3, TF_HAZARD | TF_TRIGGER },
    { 0, 0, 0, 0 },
};
static const char * const rows[18] = {
    "####################",
    "#..................#",
    "#..P......P.......F#",
    "#..................#",
    "#....x....?........#",      /* '?' has no legend entry */
    "#...\xA3..............#",   /* 0x80 | '#': the high bit is ignored */
    "#..................#",
    "#P.................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "####################",
};
static const map_def_t room = { 20, 18, rows, legend, &ts };

/* ---- 64x40 map built in RAM: 16 chars 'a'..'p' in a pattern that never repeats per 32 tiles ---- */
#define BW 64
#define BH 40
static const map_legend_t big_legend[] = {
    { 'a', 0, 0, 0 }, { 'b', 1, 1, 0 }, { 'c', 2, 2, 0 }, { 'd', 3, 3, 0 },
    { 'e', 4, 0, 0 }, { 'f', 5, 1, 0 }, { 'g', 6, 2, 0 }, { 'h', 7, 3, 0 },
    { 'i', 8, 0, 0 }, { 'j', 9, 1, 0 }, { 'k', 10, 2, 0 }, { 'l', 11, 3, 0 },
    { 'm', 12, 0, 0 }, { 'n', 13, 1, 0 }, { 'o', 14, 2, 0 }, { 'p', 15, 3, 0 },
    { 0, 0, 0, 0 },
};
static const tileset_def_t big_ts = { font_main, 16, pals, 4 };
static char grid[BH][BW];
static const char *grid_rows[BH];
static const map_def_t big = { BW, BH, grid_rows, big_legend, &big_ts };

static uint8_t big_code(uint16_t x, uint16_t y) { return (uint8_t)((x * 3 + y * 5 + (y >> 2) * 7) & 15); }

static void build_big(void) {
    uint8_t x, y;
    for (y = 0; y < BH; y++) {
        for (x = 0; x < BW; x++) grid[y][x] = (char)('a' + big_code(x, y));
        grid_rows[y] = grid[y];
    }
}

/* every cell of the 21x19 drawn area must match the map; returns number of wrong cells */
static uint16_t big_screen_errors(void) {
    uint16_t tx = (uint16_t)cam_x >> 3, ty = (uint16_t)cam_y >> 3, mx, my, err = 0;
    uint8_t dx, dy, code;
    for (dy = 0; dy < 19; dy++)
        for (dx = 0; dx < 21; dx++) {
            mx = tx + dx; my = ty + dy;
            if (mx >= BW || my >= BH) continue;
            code = big_code(mx, my);
            if (hw_bkg_tile(mx & 31, my & 31) != 128 + code) err++;
            else if (hw_bkg_attr(mx & 31, my & 31) != (code & 3)) err++;
        }
    return err;
}

static uint8_t legend_index(char c) {
    uint8_t i;
    for (i = 0; legend[i].ch; i++) if (legend[i].ch == c) return i;
    return 0xFF;
}

void unit_setup(void) BANKED {
    unit_reset_engine();
    map_load(&room, 0);
}

TEST(load_sets_pixel_size) {
    ASSERT_EQ(map_w_px, 160);
    ASSERT_EQ(map_h_px, 144);
}

TEST(load_copies_tileset_to_tile_128) {
    uint8_t i;
    VBK_REG = 1;
    map_load(&room, 0);
    ASSERT_EQ(VBK_REG & 1, 0);
    for (i = 0; i < 6 * 16; i++) ASSERT_EQ(hw_vram(0, 0x8800 + i), font_main[i]);
}

TEST(load_sets_bkg_palettes_keeps_ui) {
    uint8_t i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) ASSERT_EQ(pal_ram[(i << 2) + j], pals[i][j]);
    ASSERT_EQ(pal_ram[28], pal_ui_default[0]);
}

TEST(map_char_reads_rows) {
    ASSERT_EQ(map_char(0, 0), '#');
    ASSERT_EQ(map_char(3, 2), 'P');
    ASSERT_EQ(map_char(18, 2), 'F');
    ASSERT_EQ(map_char(4, 5), '#');       /* high bit masked */
}

TEST(map_flags_inside_and_outside) {
    ASSERT_EQ(map_flags(0, 0), TF_SOLID);
    ASSERT_EQ(map_flags(1, 1), 0);
    ASSERT_EQ(map_flags(5, 4), TF_HAZARD | TF_TRIGGER);
    ASSERT_EQ(map_flags(18, 2), TF_FLIPX | TF_FLIPY | TF_OVER);
    ASSERT_EQ(map_flags(10, 4), 0);       /* char without legend entry */
    ASSERT_EQ(map_flags(4, 5), TF_SOLID);
    ASSERT_EQ(map_flags(20, 1), TF_SOLID); /* right of the map */
    ASSERT_EQ(map_flags(1000, 1), TF_SOLID);
    ASSERT_EQ(map_flags(1, 18), 0);        /* below the map */
    ASSERT_EQ(map_flags(1, 500), 0);
}

TEST(map_flags_px_converts_and_blocks_negative) {
    ASSERT_EQ(map_flags_px(7, 7), TF_SOLID);
    ASSERT_EQ(map_flags_px(8, 8), 0);
    ASSERT_EQ(map_flags_px(47, 39), TF_HAZARD | TF_TRIGGER);
    ASSERT_EQ(map_flags_px(-1, 20), TF_SOLID);
    ASSERT_EQ(map_flags_px(20, -1), TF_SOLID);
    ASSERT_EQ(map_flags_px(160, 20), TF_SOLID);
    ASSERT_EQ(map_flags_px(20, 144), 0);
}

TEST(map_find_nth_occurrence) {
    uint16_t tx = 99, ty = 99;
    ASSERT_EQ(map_find('P', 0, &tx, &ty), 1); ASSERT_EQ(tx, 3);  ASSERT_EQ(ty, 2);
    ASSERT_EQ(map_find('P', 1, &tx, &ty), 1); ASSERT_EQ(tx, 10); ASSERT_EQ(ty, 2);
    ASSERT_EQ(map_find('P', 2, &tx, &ty), 1); ASSERT_EQ(tx, 1);  ASSERT_EQ(ty, 7);
    tx = ty = 99;
    ASSERT_EQ(map_find('P', 3, &tx, &ty), 0);
    ASSERT_EQ(map_find('Z', 0, &tx, &ty), 0);
    ASSERT_EQ(tx, 99);
    ASSERT_EQ(ty, 99);
}

TEST(cam_set_draws_whole_screen) {
    uint8_t x, y, i;
    uint8_t want_t, want_a;
    fill_bkg_rect(0, 0, 32, 32, 0x55);     /* junk: every drawn cell must be rewritten */
    cam_set(0, 0);
    ASSERT_EQ(cam_x, 0);
    ASSERT_EQ(cam_y, 0);
    ASSERT_EQ(SCX_REG, 0);
    ASSERT_EQ(SCY_REG, 0);
    for (y = 0; y < 18; y++)
        for (x = 0; x < 20; x++) {
            i = legend_index(map_char(x, y));
            want_t = TILE_BLANK; want_a = ATTR_BLANK;
            if (i != 0xFF) { want_t = 128 + legend[i].tile; want_a = legend[i].pal | (legend[i].flags & 0xE0); }
            ASSERT_EQ(hw_bkg_tile(x, y), want_t);
            ASSERT_EQ(hw_bkg_attr(x, y), want_a);
        }
    ASSERT_EQ(hw_bkg_tile(20, 5), TILE_BLANK);   /* 21st column is outside the map */
    ASSERT_EQ(hw_bkg_attr(20, 5), ATTR_BLANK);
    ASSERT_EQ(hw_bkg_tile(5, 18), TILE_BLANK);   /* 19th row too */
}

TEST(cam_set_legend_attributes) {
    cam_set(0, 0);
    ASSERT_EQ(hw_bkg_tile(18, 2), 130);
    ASSERT_EQ(hw_bkg_attr(18, 2), 2 | 0x20 | 0x40 | 0x80);
    ASSERT_EQ(hw_bkg_tile(5, 4), 133);
    ASSERT_EQ(hw_bkg_attr(5, 4), 3);
    ASSERT_EQ(hw_bkg_tile(10, 4), TILE_BLANK);
    ASSERT_EQ(hw_bkg_attr(10, 4), ATTR_BLANK);
}

TEST(cam_set_clamps_to_map) {
    cam_set(-50, 999);                     /* map fits the screen: always 0,0 */
    ASSERT_EQ(cam_x, 0);
    ASSERT_EQ(cam_y, 0);
    build_big();
    map_load(&big, 0);
    cam_set(10000, 10000);
    ASSERT_EQ(cam_x, BW * 8 - 160);
    ASSERT_EQ(cam_y, BH * 8 - 144);
    cam_set(-1, -1);
    ASSERT_EQ(cam_x, 0);
    ASSERT_EQ(cam_y, 0);
}

TEST(cam_set_mid_map_draws_and_scrolls) {
    build_big();
    map_load(&big, 0);
    cam_set(203, 77);
    ASSERT_EQ(cam_x, 203);
    ASSERT_EQ(SCX_REG, 203);
    ASSERT_EQ(SCY_REG, 77);
    ASSERT_EQ(big_screen_errors(), 0);
}

TEST(set_tile_changes_vram_only) {
    cam_set(0, 0);
    map_set_tile(2, 1, '#');
    ASSERT_EQ(hw_bkg_tile(2, 1), 129);
    ASSERT_EQ(hw_bkg_attr(2, 1), 1);
    ASSERT_EQ(map_char(2, 1), '.');        /* map data untouched */
    ASSERT_EQ(map_flags(2, 1), 0);
    map_set_tile(3, 1, 'F');
    ASSERT_EQ(hw_bkg_attr(3, 1), 0xE2);
}

TEST(set_tile_ignores_cells_off_screen) {
    build_big();
    map_load(&big, 0);
    cam_set(0, 0);
    map_set_tile(25, 3, 'a');              /* past the 21 drawn columns */
    ASSERT_EQ(hw_bkg_tile(25, 3), TILE_BLANK);
    map_set_tile(3, 20, 'a');              /* past the 19 drawn rows */
    ASSERT_EQ(hw_bkg_tile(3, 20), TILE_BLANK);
    map_set_tile(20, 18, 'a');             /* last drawn cell is allowed */
    ASSERT_EQ(hw_bkg_tile(20, 18), 128);
}

TEST(follow_dead_zone) {
    build_big();
    map_load(&big, 0);
    cam_set(100, 100);
    cam_follow(100 + 80 + 8, 100 + 72 - 8);
    ASSERT_EQ(cam_x, 100);
    ASSERT_EQ(cam_y, 100);
    cam_follow(100 + 80 + 11, 100 + 72 - 10);
    ASSERT_EQ(cam_x, 103);
    ASSERT_EQ(cam_y, 98);
    ASSERT_EQ(SCX_REG, 103);
    ASSERT_EQ(SCY_REG, 98);
}

TEST(follow_moves_at_most_8_px) {
    build_big();
    map_load(&big, 0);
    cam_set(100, 100);
    cam_follow(400, 300);
    ASSERT_EQ(cam_x, 108);
    ASSERT_EQ(cam_y, 108);
    cam_follow(0, 0);
    ASSERT_EQ(cam_x, 100);
    ASSERT_EQ(cam_y, 100);
}

TEST(follow_clamps_to_map) {
    uint8_t i;
    build_big();
    map_load(&big, 0);
    cam_set(0, 0);
    for (i = 0; i < 80; i++) cam_follow(2000, 2000);
    ASSERT_EQ(cam_x, BW * 8 - 160);
    ASSERT_EQ(cam_y, BH * 8 - 144);
    for (i = 0; i < 80; i++) cam_follow(-500, -500);
    ASSERT_EQ(cam_x, 0);
    ASSERT_EQ(cam_y, 0);
}

TEST(streaming_right_and_back) {
    uint8_t i;
    build_big();
    map_load(&big, 0);
    cam_set(0, 0);
    for (i = 0; i < 60; i++) cam_follow(400, 72);
    ASSERT_EQ(cam_x, 400 - 80 - 8);
    ASSERT_EQ(big_screen_errors(), 0);
    for (i = 0; i < 60; i++) cam_follow(0, 72);
    ASSERT_EQ(cam_x, 0);
    ASSERT_EQ(big_screen_errors(), 0);
}

TEST(streaming_down_and_up) {
    uint8_t i;
    build_big();
    map_load(&big, 0);
    cam_set(0, 0);
    for (i = 0; i < 60; i++) cam_follow(80, 1000);
    ASSERT_EQ(cam_y, BH * 8 - 144);
    ASSERT_EQ(big_screen_errors(), 0);
    for (i = 0; i < 60; i++) cam_follow(80, 0);
    ASSERT_EQ(cam_y, 0);
    ASSERT_EQ(big_screen_errors(), 0);
}

TEST(streaming_diagonal_odd_steps) {
    uint8_t i;
    build_big();
    map_load(&big, 0);
    cam_set(0, 0);
    for (i = 0; i < 90; i++) {
        cam_follow(88 + i * 3, 80 + i * 2);
        if ((i & 15) == 15) ASSERT_EQ(big_screen_errors(), 0);
    }
    ASSERT_EQ(big_screen_errors(), 0);
    for (i = 90; i > 0; i--) cam_follow(88 + i * 5 - 300, 80 + i - 60);
    ASSERT_EQ(big_screen_errors(), 0);
}

TEST(shake_offsets_scroll_then_stops) {
    uint8_t i, moved = 0;
    int8_t d;
    build_big();
    map_load(&big, 0);
    cam_set(40, 40);
    cam_shake(6, 2);
    for (i = 0; i < 6; i++) {
        cam_follow(40 + 80, 40 + 72);
        ASSERT_EQ(cam_x, 40);              /* shake never moves the camera itself */
        d = (int8_t)(SCX_REG - 40);
        ASSERT(d >= -2 && d <= 2);
        if (d) moved = 1;
        d = (int8_t)(SCY_REG - 40);
        ASSERT(d >= -2 && d <= 2);
        if (d) moved = 1;
    }
    ASSERT(moved);
    cam_follow(40 + 80, 40 + 72);
    ASSERT_EQ(SCX_REG, 40);
    ASSERT_EQ(SCY_REG, 40);
}

TEST(cam_reset_clears_camera_and_size) {
    build_big();
    map_load(&big, 0);
    cam_set(100, 50);
    cam_shake(10, 3);
    cam_reset();
    ASSERT_EQ(cam_x, 0);
    ASSERT_EQ(cam_y, 0);
    ASSERT_EQ(map_w_px, 0);
    ASSERT_EQ(map_h_px, 0);
}

TEST(banked_map_switches_and_restores) {
    uint8_t save = CURRENT_BANK;
    uint16_t tx = 0, ty = 0;
    ASSERT(BANK(map_banked) != save);
    map_load(&map_banked, BANK(map_banked));
    ASSERT_EQ(CURRENT_BANK, save);
    ASSERT_EQ(hw_vram(0, 0x8800), 0x01);
    ASSERT_EQ(hw_vram(0, 0x881F), 0xEE);
    ASSERT_EQ(pal_ram[1], RGB(3, 5, 7));
    ASSERT_EQ(map_char(8, 2), 'K');
    ASSERT_EQ(CURRENT_BANK, save);
    ASSERT_EQ(map_flags(0, 0), TF_SOLID);
    ASSERT_EQ(map_find('K', 0, &tx, &ty), 1);
    ASSERT_EQ(tx, 8);
    ASSERT_EQ(ty, 2);
    cam_set(0, 0);
    ASSERT_EQ(CURRENT_BANK, save);
    ASSERT_EQ(hw_bkg_tile(0, 0), 129);
    ASSERT_EQ(hw_bkg_tile(1, 1), 128);
}

void unit_tests(void) BANKED {
    RUN(load_sets_pixel_size);
    RUN(load_copies_tileset_to_tile_128);
    RUN(load_sets_bkg_palettes_keeps_ui);
    RUN(map_char_reads_rows);
    RUN(map_flags_inside_and_outside);
    RUN(map_flags_px_converts_and_blocks_negative);
    RUN(map_find_nth_occurrence);
    RUN(cam_set_draws_whole_screen);
    RUN(cam_set_legend_attributes);
    RUN(cam_set_clamps_to_map);
    RUN(cam_set_mid_map_draws_and_scrolls);
    RUN(set_tile_changes_vram_only);
    RUN(set_tile_ignores_cells_off_screen);
    RUN(follow_dead_zone);
    RUN(follow_moves_at_most_8_px);
    RUN(follow_clamps_to_map);
    RUN(streaming_right_and_back);
    RUN(streaming_down_and_up);
    RUN(streaming_diagonal_odd_steps);
    RUN(shake_offsets_scroll_then_stops);
    RUN(cam_reset_clears_camera_and_size);
    RUN(banked_map_switches_and_restores);
}
