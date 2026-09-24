#include "engine/engine.h"

palette_color_t pal_ram[64];
static uint8_t fade_level, fade_white;
static palette_color_t tmp[64];

static palette_color_t blend(palette_color_t c, uint8_t s, uint8_t white) {
    uint8_t r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    if (white) { r += ((31 - r) * s) >> 3; g += ((31 - g) * s) >> 3; b += ((31 - b) * s) >> 3; }
    else       { r -= (r * s) >> 3;        g -= (g * s) >> 3;        b -= (b * s) >> 3; }
    return RGB(r, g, b);
}

void fade_apply(void) {
    uint8_t i;
    for (i = 0; i < 64; i++) tmp[i] = fade_level ? blend(pal_ram[i], fade_level, fade_white) : pal_ram[i];
    set_bkg_palette(0, 8, tmp);
    set_sprite_palette(0, 8, tmp + 32);
}

void fade_init(void) {
    uint8_t i;
    for (i = 0; i < 64; i++) pal_ram[i] = 0;
    fade_level = 8; fade_white = 0;
    fade_apply();
}

void fade_set_level(uint8_t level) {
    fade_level = level;
    fade_apply();
}

void fade_out(uint8_t fps, uint8_t to_white) {
    uint8_t s, f;
    fade_white = to_white;
    for (s = 1; s <= 8; s++) { fade_level = s; vsync(); fade_apply(); for (f = 1; f < fps; f++) vsync(); }
}

void fade_in(uint8_t fps, uint8_t from_white) {
    uint8_t s, f;
    fade_white = from_white;
    s = 8;
    do {
        s--;
        fade_level = s; vsync(); fade_apply(); for (f = 1; f < fps; f++) vsync();
    } while (s);
}
