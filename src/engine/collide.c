#include "engine/engine.h"

uint8_t rect_overlap(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah,
                     int16_t bx, int16_t by, uint8_t bw, uint8_t bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

uint8_t body_overlap(const body_t *a, const body_t *b) {
    return rect_overlap(UNFIX(a->x), UNFIX(a->y), a->w, a->h, UNFIX(b->x), UNFIX(b->y), b->w, b->h);
}

static uint8_t col_hit(int16_t x, int16_t y, uint8_t h, uint8_t mask) {   /* vertical edge */
    int16_t yy = y, end = y + h - 1;
    for (;;) {
        if (map_flags_px(x, yy) & mask) return 1;
        if (yy == end) return 0;
        yy += 8; if (yy > end) yy = end;
    }
}

static uint8_t row_hit(int16_t x, int16_t y, uint8_t w, uint8_t mask) {   /* horizontal edge */
    int16_t xx = x, end = x + w - 1;
    for (;;) {
        if (map_flags_px(xx, y) & mask) return 1;
        if (xx == end) return 0;
        xx += 8; if (xx > end) xx = end;
    }
}

uint8_t body_move(body_t *b) {
    uint8_t hit = 0, mask;
    int16_t px, py, bottom, old_bottom;

    b->x += b->vx;
    px = UNFIX(b->x); py = UNFIX(b->y);
    if (b->vx > 0 && col_hit(px + b->w - 1, py, b->h, TF_SOLID)) {
        px = ((px + b->w - 1) & ~7) - b->w; b->x = FIX(px); b->vx = 0; hit |= HIT_RIGHT;
    } else if (b->vx < 0 && col_hit(px, py, b->h, TF_SOLID)) {
        px = (px & ~7) + 8; b->x = FIX(px); b->vx = 0; hit |= HIT_LEFT;
    }

    old_bottom = py + b->h - 1;
    b->y += b->vy;
    py = UNFIX(b->y);
    if (b->vy > 0) {
        bottom = py + b->h - 1;
        mask = TF_SOLID;
        if ((old_bottom >> 3) < (bottom >> 3)) mask |= TF_ONEWAY;   /* entered the row from above */
        if (row_hit(px, bottom, b->w, mask)) {
            py = (bottom & ~7) - b->h; b->y = FIX(py); b->vy = 0; hit |= HIT_DOWN;
        }
    } else if (b->vy < 0 && row_hit(px, py, b->w, TF_SOLID)) {
        py = (py & ~7) + 8; b->y = FIX(py); b->vy = 0; hit |= HIT_UP;
    }
    return hit;
}

uint8_t body_on_ground(const body_t *b) {
    return row_hit(UNFIX(b->x), UNFIX(b->y) + b->h, b->w, TF_SOLID | TF_ONEWAY);
}

uint8_t body_touch_flags(const body_t *b) {
    int16_t x0 = UNFIX(b->x), y0 = UNFIX(b->y);
    int16_t x1 = x0 + b->w - 1, y1 = y0 + b->h - 1;
    int16_t x, y;
    uint8_t f = 0;
    for (y = y0; ; ) {
        for (x = x0; ; ) {
            f |= map_flags_px(x, y);
            if (x == x1) break;
            x += 8; if (x > x1) x = x1;
        }
        if (y == y1) break;
        y += 8; if (y > y1) y = y1;
    }
    return f;
}
