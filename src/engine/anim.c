#include "engine/engine.h"

void anim_play(anim_t *a, const anim_def_t *def) {
    if (a->def == def) return;
    a->def = def; a->i = 0; a->timer = 0; a->done = 0;
}

void anim_update(anim_t *a) {
    if (!a->def || a->done) return;
    if (++a->timer < a->def->speed) return;
    a->timer = 0;
    if (a->i + 1 < a->def->len) a->i++;
    else if (a->def->loop) a->i = 0;
    else a->done = 1;
}
