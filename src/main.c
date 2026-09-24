#include "engine/engine.h"
#include "scenes/scenes.h"

void main(void) {
    engine_init();                 /* CGB check, fast CPU, VRAM clear, font, audio, palettes black */
    scene_start(&scene_title);     /* enter() then fade in */
    while (1) {
        vsync();                   /* wait for VBlank: 60 frames per second */
        input_update();
        scene_update();            /* spr_begin(); cur->update(); spr_end(); + pending transitions */
    }
}
