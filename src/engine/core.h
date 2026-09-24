#ifndef CORE_H
#define CORE_H
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <rand.h>
#include <gbdk/emu_debug.h>

/* fixed point 12.4 in int16_t: 1 pixel = 16 units, range +-2047 px */
#define FIX(px)     ((int16_t)((px) << 4))
#define UNFIX(v)    ((int16_t)(v) >> 4)
#define ABS(a)      ((a) < 0 ? -(a) : (a))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define CLAMP(v,lo,hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

void engine_init(void);
int16_t approach(int16_t v, int16_t target, int16_t step);  /* move v toward target by step */
void rand_seed(void);     /* call when the player presses START */
#endif
