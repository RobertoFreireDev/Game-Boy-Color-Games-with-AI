#include "engine/engine.h"

uint8_t keys, keys_prev;

void input_update(void) {
    keys_prev = keys;
    keys = joypad();
}

uint8_t input_wait_press(uint8_t mask) {
    uint8_t k;
    for (;;) {
        vsync();
        input_update();
        k = keys & ~keys_prev & mask;
        if (k) return k;
    }
}
