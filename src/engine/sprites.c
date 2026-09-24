#include "engine/engine.h"

static uint8_t spr_next, spr_last;

void spr_begin(void) { spr_next = 0; }

void spr_end(void) {
    uint8_t i;
    for (i = spr_next; i < spr_last; i++) hide_sprite(i);
    spr_last = spr_next;
}

uint8_t spr_put(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop) {
    if (spr_next >= 40 || sx <= -8 || sx >= 160 || sy <= -8 || sy >= 144) return 0;
    set_sprite_tile(spr_next, tile);
    set_sprite_prop(spr_next, prop);
    move_sprite(spr_next, (uint8_t)(sx + 8), (uint8_t)(sy + 16));
    spr_next++;
    return 1;
}

void spr_draw(const sprite_t *s, uint8_t frame, int16_t sx, int16_t sy, uint8_t flags) {
    uint8_t r, c, sr, sc;
    uint8_t first = s->base + frame * s->tpf;
    uint8_t prop = s->pal | flags;
    for (r = 0; r < s->h; r++) {
        sr = (flags & SPR_FLIPY) ? (uint8_t)(s->h - 1 - r) : r;
        for (c = 0; c < s->w; c++) {
            sc = (flags & SPR_FLIPX) ? (uint8_t)(s->w - 1 - c) : c;
            spr_put(first + sr * s->w + sc, sx + (c << 3), sy + (r << 3), prop);
        }
    }
}

void spr_hide_all(void) {
    uint8_t i;
    for (i = 0; i < 40; i++) hide_sprite(i);
    spr_next = spr_last = 0;
}
