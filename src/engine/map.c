#include "engine/engine.h"

static uint8_t lut_tile[128], lut_attr[128], lut_flags[128];
static const char * const *map_rows;
static uint16_t map_w, map_h;
static uint8_t map_bank;
uint16_t map_w_px, map_h_px;

int16_t cam_x, cam_y;
static uint16_t drawn_tx, drawn_ty;   /* tile of the currently drawn top-left */
static int8_t shake_dx, shake_dy;
static uint8_t shake_t, shake_s;

void map_load(const map_def_t *m, uint8_t bank) {
    const map_legend_t *l;
    const tileset_def_t *ts;
    uint8_t i, c;
    uint8_t save = CURRENT_BANK;
    map_bank = bank;
    if (bank) SWITCH_ROM(bank);
    for (i = 0; i < 128; i++) { lut_tile[i] = TILE_BLANK; lut_attr[i] = ATTR_BLANK; lut_flags[i] = 0; }
    for (l = m->legend; l->ch; l++) {
        c = (uint8_t)l->ch & 0x7F;
        lut_tile[c] = 128 + l->tile;
        lut_attr[c] = l->pal | (l->flags & 0xE0);
        lut_flags[c] = l->flags;
    }
    map_w = m->w; map_h = m->h; map_rows = m->rows;
    map_w_px = map_w << 3; map_h_px = map_h << 3;
    ts = m->tileset;
    VBK_REG = 0;
    set_bkg_data(128, ts->count, ts->tiles);
    for (i = 0; i < ts->pal_count; i++) gfx_set_bkg_palette(i, ts->pals[i]);
    if (bank) SWITCH_ROM(save);
}

char map_char(uint16_t tx, uint16_t ty) {
    char c;
    uint8_t save = CURRENT_BANK;
    if (map_bank) SWITCH_ROM(map_bank);
    c = map_rows[ty][tx] & 0x7F;
    if (map_bank) SWITCH_ROM(save);
    return c;
}

uint8_t map_flags(uint16_t tx, uint16_t ty) {
    if (tx >= map_w) return TF_SOLID;
    if (ty >= map_h) return 0;
    return lut_flags[(uint8_t)map_char(tx, ty)];
}

uint8_t map_flags_px(int16_t px, int16_t py) {
    if (px < 0 || py < 0) return TF_SOLID;
    return map_flags((uint16_t)px >> 3, (uint16_t)py >> 3);
}

uint8_t map_find(char ch, uint8_t n, uint16_t *tx, uint16_t *ty) {
    uint16_t x, y;
    for (y = 0; y < map_h; y++)
        for (x = 0; x < map_w; x++)
            if (map_char(x, y) == ch) {
                if (n == 0) { *tx = x; *ty = y; return 1; }
                n--;
            }
    return 0;
}

static void put_raw(uint16_t mx, uint16_t my, uint8_t t, uint8_t a) {
    VBK_REG = 1; set_bkg_tile_xy(mx & 31, my & 31, a);
    VBK_REG = 0; set_bkg_tile_xy(mx & 31, my & 31, t);
}

void map_set_tile(uint16_t tx, uint16_t ty, char ch) {
    uint8_t c = (uint8_t)ch & 0x7F;
    if (tx < drawn_tx || tx > drawn_tx + 20 || ty < drawn_ty || ty > drawn_ty + 18) return;
    put_raw(tx, ty, lut_tile[c], lut_attr[c]);
}

/* ---------- camera + streaming (the BG map is 32x32 and wraps) ---------- */

static void put_cell(uint16_t mx, uint16_t my) {
    uint8_t t = TILE_BLANK, a = ATTR_BLANK, c;
    if (mx < map_w && my < map_h) { c = (uint8_t)map_char(mx, my); t = lut_tile[c]; a = lut_attr[c]; }
    put_raw(mx, my, t, a);
}
static void draw_col(uint16_t mx, uint16_t my) { uint8_t i; for (i = 0; i < 19; i++) put_cell(mx, my + i); }
static void draw_row(uint16_t mx, uint16_t my) { uint8_t i; for (i = 0; i < 21; i++) put_cell(mx + i, my); }

static void cam_clamp(void) {
    int16_t mx = (int16_t)map_w_px - 160, my = (int16_t)map_h_px - 144;
    if (mx < 0) mx = 0;
    if (my < 0) my = 0;
    cam_x = CLAMP(cam_x, 0, mx);
    cam_y = CLAMP(cam_y, 0, my);
}

static void cam_apply(void) {          /* after cam_x/cam_y changed and were clamped */
    uint16_t tx = cam_x >> 3, ty = cam_y >> 3;
    uint8_t s2;
    while (drawn_tx < tx) { drawn_tx++; draw_col(drawn_tx + 20, drawn_ty); }
    while (drawn_tx > tx) { drawn_tx--; draw_col(drawn_tx,      drawn_ty); }
    while (drawn_ty < ty) { drawn_ty++; draw_row(drawn_tx, drawn_ty + 18); }
    while (drawn_ty > ty) { drawn_ty--; draw_row(drawn_tx, drawn_ty); }
    if (shake_t) {
        shake_t--;
        s2 = (shake_s << 1) + 1;
        shake_dx = (int8_t)(rand() % s2) - (int8_t)shake_s;
        shake_dy = (int8_t)(rand() % s2) - (int8_t)shake_s;
    } else {
        shake_dx = shake_dy = 0;
    }
    move_bkg((uint8_t)(cam_x + shake_dx), (uint8_t)(cam_y + shake_dy));
}

void cam_set(int16_t x, int16_t y) {
    uint8_t i;
    cam_x = x; cam_y = y;
    cam_clamp();
    drawn_tx = cam_x >> 3; drawn_ty = cam_y >> 3;
    for (i = 0; i < 19; i++) draw_row(drawn_tx, drawn_ty + i);
    cam_apply();
}

void cam_follow(int16_t wx, int16_t wy) {
    int16_t dx = wx - 80 - cam_x, dy = wy - 72 - cam_y;
    int16_t nx = cam_x, ny = cam_y;
    if (dx > 8) nx += dx - 8; else if (dx < -8) nx += dx + 8;
    if (dy > 8) ny += dy - 8; else if (dy < -8) ny += dy + 8;
    cam_x = CLAMP(nx, cam_x - 8, cam_x + 8);
    cam_y = CLAMP(ny, cam_y - 8, cam_y + 8);
    cam_clamp();
    cam_apply();
}

void cam_shake(uint8_t frames, uint8_t strength) { shake_t = frames; shake_s = strength; }

void cam_reset(void) {
    cam_x = cam_y = 0;
    drawn_tx = drawn_ty = 0;
    shake_t = 0; shake_dx = shake_dy = 0;
    map_w = map_h = 0; map_w_px = map_h_px = 0;
}
