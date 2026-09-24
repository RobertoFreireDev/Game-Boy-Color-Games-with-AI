#ifndef ANIM_H
#define ANIM_H
#include "core.h"
typedef struct { const uint8_t *frames; uint8_t len; uint8_t speed; uint8_t loop; } anim_def_t; /* speed = frames per step */
typedef struct { const anim_def_t *def; uint8_t i, timer, done; } anim_t;
void anim_play(anim_t *a, const anim_def_t *def);   /* restarts only if def changed */
void anim_update(anim_t *a);                        /* once per frame */
#define anim_frame(a) ((a)->def->frames[(a)->i])
#endif
