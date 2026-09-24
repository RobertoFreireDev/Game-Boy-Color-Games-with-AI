#ifndef TWEEN_H
#define TWEEN_H
#include "core.h"
#define EASE_LINEAR 0
#define EASE_IN 1
#define EASE_OUT 2
#define EASE_INOUT 3
uint8_t tween_start(int16_t *target, int16_t to, uint8_t frames, uint8_t ease);  /* from = *target now */
void tween_update(void);            /* once per frame */
uint8_t tween_busy(const int16_t *target);
void tween_clear(void);             /* pool of 8 */
#endif
