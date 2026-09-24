#ifndef INPUT_H
#define INPUT_H
#include "core.h"
extern uint8_t keys, keys_prev;
void input_update(void);
#define KEY_HELD(k)     (keys & (k))
#define KEY_PRESSED(k)  ((keys & (k)) && !(keys_prev & (k)))
#define KEY_RELEASED(k) (!(keys & (k)) && (keys_prev & (k)))
uint8_t input_wait_press(uint8_t mask);        /* blocking; returns the key pressed */
#endif
