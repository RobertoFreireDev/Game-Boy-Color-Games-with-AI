#include "engine/engine.h"

void engine_init(void) {
    if (_cpu != CGB_TYPE) { while (1) vsync(); }   /* Color only */
    cpu_fast();
    DISPLAY_OFF;                 /* the only allowed DISPLAY_OFF */
    SPRITES_8x8;
    fade_init();                 /* all palettes in RAM, fade level = fully black */
    scene_reset_screen();        /* clear BG map + attributes, hide sprites/window */
    text_init();                 /* font + box tiles into VRAM bank 1, default UI palette */
    audio_init();                /* sound on, add_VBL(audio_update) */
    SHOW_BKG; SHOW_SPRITES; HIDE_WIN;
    DISPLAY_ON;
}

int16_t approach(int16_t v, int16_t target, int16_t step) {
    if (v < target) { v += step; if (v > target) v = target; }
    else if (v > target) { v -= step; if (v < target) v = target; }
    return v;
}

void rand_seed(void) {
    initrand(DIV_REG | (sys_time << 8));
}
