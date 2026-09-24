#ifndef COLLIDE_H
#define COLLIDE_H
#include "core.h"
typedef struct { int16_t x, y, vx, vy; uint8_t w, h; } body_t;   /* x,y,vx,vy fixed 12.4; w,h hitbox px */
#define HIT_LEFT 1
#define HIT_RIGHT 2
#define HIT_UP 4
#define HIT_DOWN 8
uint8_t rect_overlap(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah,
                     int16_t bx, int16_t by, uint8_t bw, uint8_t bh);   /* pixels */
uint8_t body_overlap(const body_t *a, const body_t *b);
uint8_t body_move(body_t *b);           /* moves with tile collision, returns HIT_* bits */
uint8_t body_on_ground(const body_t *b);
uint8_t body_touch_flags(const body_t *b); /* OR of flags of all tiles the hitbox covers */
#endif
