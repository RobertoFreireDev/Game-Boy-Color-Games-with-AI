/* Test assets pinned to ROM bank 2 (test code is autobanked into bank 1), for gfx_load_sprite / map_load
   with a bank argument. Tests compare against the literal values written here. */
#pragma bank 2
#include "engine/engine.h"
#include "data_banked.h"

BANKREF(spr_banked)
static const uint8_t banked_tiles[2 * 16] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
    0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,0xEE,
};
static const palette_color_t banked_pal[4] = { RGB(0,0,0), RGB(3,5,7), RGB(9,11,13), RGB(15,17,19) };
const sprite_def_t spr_banked = { banked_tiles, 1, 1, 2, banked_pal, 5 };

BANKREF(map_banked)
static const palette_color_t * const banked_pals[1] = { banked_pal };
static const tileset_def_t ts_banked = { banked_tiles, 2, banked_pals, 1 };
static const map_legend_t banked_legend[] = {
    { '.', 0, 0, 0 },
    { '#', 1, 0, TF_SOLID },
    { 'K', 0, 0, TF_SPAWN },
    { 0, 0, 0, 0 },
};
static const char * const banked_rows[18] = {
    "####################",
    "#..................#",
    "#.......K..........#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
    "#..................#",
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
const map_def_t map_banked = { 20, 18, banked_rows, banked_legend, &ts_banked };
