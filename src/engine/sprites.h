#ifndef SPRITES_H
#define SPRITES_H
#include "gfx.h"
#define SPR_FLIPX 0x20
#define SPR_FLIPY 0x40
#define SPR_BEHIND 0x80
void spr_begin(void);
void spr_end(void);
uint8_t spr_put(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop);   /* one 8x8, screen coords */
void spr_draw(const sprite_t *s, uint8_t frame, int16_t sx, int16_t sy, uint8_t flags);
void spr_hide_all(void);
#endif
