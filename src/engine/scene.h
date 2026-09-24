#ifndef SCENE_H
#define SCENE_H
#include "core.h"
typedef struct { void (*enter)(void); void (*update)(void); void (*leave)(void); } scene_t;  /* leave may be 0 */
#define TRANS_NONE 0
#define TRANS_FADE_BLACK 1
#define TRANS_FADE_WHITE 2
void scene_start(const scene_t *first);
void scene_goto(const scene_t *next, uint8_t transition);   /* performed at end of this frame */
void scene_update(void);
void scene_reset_screen(void);
#endif
