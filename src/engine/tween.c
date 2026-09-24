#include "engine/engine.h"

#define MAX_TWEENS 8

typedef struct {
    int16_t *target;
    int16_t from, to;
    uint8_t t, dur, ease;
} tween_t;

static tween_t tweens[MAX_TWEENS];

static const uint16_t ease_tbl[4][17] = {
  {0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256},
  {0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225,256},
  {0,31,60,87,112,135,156,175,192,207,220,231,240,247,252,255,256},
  {0,2,8,18,32,50,72,98,128,158,184,206,224,238,248,254,256},
};

uint8_t tween_start(int16_t *target, int16_t to, uint8_t frames, uint8_t ease) {
    uint8_t i, free_i = 0xFF;
    for (i = 0; i < MAX_TWEENS; i++) {
        if (tweens[i].target == target) { free_i = i; break; }   /* replace a tween on the same value */
        if (!tweens[i].target && free_i == 0xFF) free_i = i;
    }
    if (free_i == 0xFF) return 0;
    if (!frames) frames = 1;
    tweens[free_i].target = target;
    tweens[free_i].from = *target;
    tweens[free_i].to = to;
    tweens[free_i].t = 0;
    tweens[free_i].dur = frames;
    tweens[free_i].ease = ease & 3;
    return 1;
}

void tween_update(void) {
    uint8_t i, k;
    uint16_t p, e;
    const uint16_t *tbl;
    tween_t *tw;
    for (i = 0; i < MAX_TWEENS; i++) {
        tw = &tweens[i];
        if (!tw->target) continue;
        tw->t++;
        if (tw->t >= tw->dur) {
            *tw->target = tw->to;
            tw->target = 0;
            continue;
        }
        p = ((uint16_t)tw->t << 8) / tw->dur;
        k = (uint8_t)(p >> 4);
        tbl = ease_tbl[tw->ease];
        e = tbl[k] + (((tbl[k + 1] - tbl[k]) * (p & 15)) >> 4);
        *tw->target = tw->from + (int16_t)(((int32_t)(tw->to - tw->from) * e) >> 8);
    }
}

uint8_t tween_busy(const int16_t *target) {
    uint8_t i;
    for (i = 0; i < MAX_TWEENS; i++) if (tweens[i].target == target) return 1;
    return 0;
}

void tween_clear(void) {
    uint8_t i;
    for (i = 0; i < MAX_TWEENS; i++) tweens[i].target = 0;
}
