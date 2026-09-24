#ifndef PARTICLES_H
#define PARTICLES_H
#include "gfx.h"
typedef struct {
    const sprite_t *spr;   /* 1x1 sprite; frames play across the lifetime */
    uint8_t speed;         /* max initial speed, 1/16 px per frame (24 = 1.5 px) */
    int8_t  up;            /* added to initial vy (negative = upward burst) */
    int8_t  gravity;       /* added to vy every frame */
    uint8_t life;          /* frames */
} particle_style_t;
void particles_emit(int16_t wx, int16_t wy, uint8_t count, const particle_style_t *st);  /* world px */
void particles_update(void);   /* move + draw */
void particles_clear(void);    /* pool of 12; emitting when full recycles the oldest */
#endif
