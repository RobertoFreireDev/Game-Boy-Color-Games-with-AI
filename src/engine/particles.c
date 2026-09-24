#include "engine/engine.h"

#define MAX_PARTICLES 12

typedef struct {
    const particle_style_t *st;
    int16_t x, y, vx, vy;     /* fixed 12.4, world */
    uint8_t age;
    uint8_t frame, frame_timer, frame_step;
} particle_t;

static particle_t parts[MAX_PARTICLES];
static uint8_t next_slot;

static int8_t rand_vel(uint8_t speed) {
    if (!speed) return 0;
    return (int8_t)(rand() % (uint8_t)(2 * speed + 1)) - (int8_t)speed;
}

void particles_emit(int16_t wx, int16_t wy, uint8_t count, const particle_style_t *st) {
    particle_t *p;
    uint8_t frames = st->spr->frames;
    while (count--) {
        p = &parts[next_slot];
        if (++next_slot >= MAX_PARTICLES) next_slot = 0;     /* round robin = recycles the oldest */
        p->st = st;
        p->x = FIX(wx); p->y = FIX(wy);
        p->vx = rand_vel(st->speed);
        p->vy = rand_vel(st->speed) + st->up;
        p->age = 0;
        p->frame = 0; p->frame_timer = 0;
        p->frame_step = frames > 1 ? st->life / frames : 0xFF;
        if (!p->frame_step) p->frame_step = 1;
    }
}

void particles_update(void) {
    uint8_t i;
    particle_t *p;
    for (i = 0; i < MAX_PARTICLES; i++) {
        p = &parts[i];
        if (!p->st) continue;
        if (++p->age >= p->st->life) { p->st = 0; continue; }
        p->vy += p->st->gravity;
        p->x += p->vx; p->y += p->vy;
        if (++p->frame_timer >= p->frame_step) {
            p->frame_timer = 0;
            if (p->frame + 1 < p->st->spr->frames) p->frame++;
        }
        spr_draw(p->st->spr, p->frame, W2S_X(UNFIX(p->x)), W2S_Y(UNFIX(p->y)), 0);
    }
}

void particles_clear(void) {
    uint8_t i;
    for (i = 0; i < MAX_PARTICLES; i++) parts[i].st = 0;
    next_slot = 0;
}
