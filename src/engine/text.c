#include "engine/engine.h"

extern const uint8_t font_main[], font_box_tiles[];
extern const sfx_t sfx_menu;

/* 0 = box background, 1 = shadow, 2 = border, 3 = text */
static const palette_color_t pal_ui_default[4] = {
    RGB8(24, 24, 40), RGB8(8, 8, 16), RGB8(112, 136, 216), RGB8(248, 248, 232)
};

static uint8_t buf[16];
static uint8_t num_buf[6];

void text_init(void) {
    uint8_t i, r;
    const uint8_t *g = font_main;
    VBK_REG = 1;
    for (i = 0; i < 96; i++) {
        for (r = 0; r < 8; r++) { buf[r << 1] = *g; buf[(r << 1) + 1] = *g; g++; }
        set_bkg_data(128 + i, 1, buf);
    }
    set_bkg_data(BOX_TILE0, 10, font_box_tiles);
    VBK_REG = 0;
    text_set_colors(pal_ui_default);
}

void text_set_colors(const palette_color_t *c4) { gfx_set_bkg_palette(7, c4); }

static void bkg_put(uint8_t x, uint8_t y, uint8_t tile, uint8_t attr) {
    VBK_REG = 1; set_bkg_tile_xy(x, y, attr);
    VBK_REG = 0; set_bkg_tile_xy(x, y, tile);
}

static void win_put(uint8_t x, uint8_t y, uint8_t tile) {
    VBK_REG = 1; set_win_tile_xy(x, y, ATTR_BLANK);
    VBK_REG = 0; set_win_tile_xy(x, y, tile);
}

void text_print(uint8_t x, uint8_t y, const char *s) {
    uint8_t ox = (uint8_t)(cam_x >> 3), oy = (uint8_t)(cam_y >> 3);
    uint8_t cx = x;
    while (*s) {
        if (*s == '\n') { cx = x; y++; }
        else { bkg_put((cx + ox) & 31, (y + oy) & 31, FONT_TILE(*s), ATTR_BLANK); cx++; }
        s++;
    }
}

void text_print_win(uint8_t x, uint8_t y, const char *s) {
    uint8_t cx = x;
    while (*s) {
        if (*s == '\n') { cx = x; y++; }
        else { win_put(cx, y, FONT_TILE(*s)); cx++; }
        s++;
    }
}

static const char *num_str(uint16_t n, uint8_t digits) {
    uint8_t i;
    if (digits > 5) digits = 5;
    num_buf[digits] = 0;
    for (i = digits; i > 0; i--) { num_buf[i - 1] = '0' + (uint8_t)(n % 10); n /= 10; }
    return (const char *)num_buf;
}

void text_print_num(uint8_t x, uint8_t y, uint16_t n, uint8_t digits) { text_print(x, y, num_str(n, digits)); }
void text_print_num_win(uint8_t x, uint8_t y, uint16_t n, uint8_t digits) { text_print_win(x, y, num_str(n, digits)); }

void text_clear(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t ox = (uint8_t)(cam_x >> 3), oy = (uint8_t)(cam_y >> 3);
    uint8_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            bkg_put((x + i + ox) & 31, (y + j + oy) & 31, TILE_BLANK, ATTR_BLANK);
}

void box_draw_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i, j, t;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            t = 4;                                      /* fill */
            if (j == 0) t = 1; else if (j == h - 1) t = 7;
            if (i == 0) t--; else if (i == w - 1) t++;
            win_put(x + i, y + j, BOX_TILE(t));
        }
    }
}

/* ---------- dialog ---------- */

#define DLG_COLS 18
#define DLG_ROWS 3

static void wait_frames_typing(void) {
    vsync(); input_update();
    if (!KEY_HELD(J_A)) { vsync(); input_update(); }
}

static void wait_next(void) {
    uint8_t t = 0;
    for (;;) {
        vsync(); input_update();
        win_put(18, 4, BOX_TILE((t & 16) ? 7 : 9));
        t++;
        if (KEY_PRESSED(J_A)) break;
    }
    win_put(18, 4, BOX_TILE(7));
    sfx_play(&sfx_menu);
}

static void dialog_clear(void) {
    uint8_t i, j;
    for (j = 1; j <= DLG_ROWS; j++)
        for (i = 1; i <= DLG_COLS; i++) win_put(i, j, BOX_TILE(4));
}

void dialog_show(const char *text) {
    const char *p = text, *q;
    uint8_t cx = 0, cy = 0, len;
    char c;
    box_draw_win(0, 0, 20, 5);
    move_win(7, 104);
    SHOW_WIN;
    for (;;) {
        c = *p;
        if (c == 0 || c == '\f' || cy >= DLG_ROWS) {
            wait_next();
            if (c == 0) break;
            dialog_clear();
            cx = 0; cy = 0;
            if (c == '\f') p++;
            continue;
        }
        if (c == '\n') { cx = 0; cy++; p++; continue; }
        if (c == ' ' && cx == 0) { p++; continue; }
        if (c != ' ') {                               /* word wrap */
            len = 0;
            for (q = p; *q && *q != ' ' && *q != '\n' && *q != '\f'; q++) len++;
            if (cx > 0 && cx + len > DLG_COLS) { cx = 0; cy++; continue; }
        }
        if (cx >= DLG_COLS) { cx = 0; cy++; continue; }
        win_put(1 + cx, 1 + cy, FONT_TILE(c));
        cx++; p++;
        wait_frames_typing();
    }
    HIDE_WIN;
}

static void draw_cursor_win(uint8_t y0, uint8_t sel, uint8_t count) {
    uint8_t i;
    for (i = 0; i < count; i++) win_put(1, y0 + i, i == sel ? FONT_TILE('>') : TILE_BLANK);
}

uint8_t dialog_choice(const char *prompt, const char * const *options, uint8_t count) {
    uint8_t h = count + 3, sel = 0, i;
    box_draw_win(0, 0, 20, h);
    text_print_win(1, 1, prompt);
    for (i = 0; i < count; i++) text_print_win(3, 2 + i, options[i]);
    move_win(7, 144 - (h << 3));
    SHOW_WIN;
    draw_cursor_win(2, sel, count);
    for (;;) {
        vsync(); input_update();
        if (KEY_PRESSED(J_UP))   { sel = sel ? sel - 1 : count - 1; draw_cursor_win(2, sel, count); sfx_play(&sfx_menu); }
        if (KEY_PRESSED(J_DOWN)) { sel = (sel + 1 < count) ? sel + 1 : 0; draw_cursor_win(2, sel, count); sfx_play(&sfx_menu); }
        if (KEY_PRESSED(J_A | J_START)) break;
    }
    sfx_play(&sfx_menu);
    HIDE_WIN;
    return sel;
}

uint8_t menu_run(uint8_t x, uint8_t y, const char * const *options, uint8_t count) {
    uint8_t sel = 0, i;
    for (i = 0; i < count; i++) text_print(x + 2, y + i, options[i]);
    for (;;) {
        for (i = 0; i < count; i++) text_print(x, y + i, i == sel ? ">" : " ");
        do { vsync(); input_update(); }
        while (!KEY_PRESSED(J_UP | J_DOWN | J_A | J_START));
        if (KEY_PRESSED(J_A | J_START)) break;
        if (KEY_PRESSED(J_UP)) sel = sel ? sel - 1 : count - 1;
        else sel = (sel + 1 < count) ? sel + 1 : 0;
        sfx_play(&sfx_menu);
    }
    sfx_play(&sfx_menu);
    return sel;
}
