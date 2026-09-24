#ifndef GFX_H
#define GFX_H
#include "core.h"

typedef struct {
    const uint8_t *tiles;       /* 2bpp data: frame 0 tiles, then frame 1 tiles, ... */
    uint8_t w, h;               /* frame size in 8x8 tiles */
    uint8_t frames;
    const palette_color_t *pal; /* 4 colors, color 0 transparent */
    uint8_t pal_slot;           /* OBJ palette 0-7 */
} sprite_def_t;

typedef struct { uint8_t base, w, h, tpf, frames, pal; } sprite_t;   /* RAM handle (tpf = tiles per frame) */

void gfx_set_bkg_palette(uint8_t slot, const palette_color_t *c4);  /* goes through fade */
void gfx_set_obj_palette(uint8_t slot, const palette_color_t *c4);
uint8_t gfx_load_sprite(sprite_t *out, const sprite_def_t *def, uint8_t bank);
void gfx_reset(void);
#endif
