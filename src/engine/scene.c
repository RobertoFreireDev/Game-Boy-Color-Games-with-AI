#include "engine/engine.h"

static const scene_t *cur, *pending;
static uint8_t pending_trans;

static void run_frame(void) { spr_begin(); cur->update(); spr_end(); }

void scene_reset_screen(void) {
    spr_hide_all();
    HIDE_WIN;
    VBK_REG = 1;
    fill_bkg_rect(0, 0, 32, 32, ATTR_BLANK);
    fill_win_rect(0, 0, 32, 32, ATTR_BLANK);
    VBK_REG = 0;
    fill_bkg_rect(0, 0, 32, 32, TILE_BLANK);
    fill_win_rect(0, 0, 32, 32, TILE_BLANK);
    move_bkg(0, 0);
}

void scene_start(const scene_t *first) {
    cur = first; cur->enter(); run_frame(); fade_in(4, 0);
}

void scene_goto(const scene_t *next, uint8_t transition) {
    pending = next;
    pending_trans = transition;
}

void scene_update(void) {
    uint8_t white;
    run_frame();
    if (!pending) return;
    white = (pending_trans == TRANS_FADE_WHITE);
    if (pending_trans != TRANS_NONE) fade_out(4, white);
    if (cur->leave) cur->leave();
    scene_reset_screen(); gfx_reset(); tween_clear(); particles_clear(); cam_reset();
    cur = pending; pending = 0;
    keys_prev = keys;           /* the press that caused the transition must not count again in the new scene */
    cur->enter();
    run_frame();
    if (pending_trans != TRANS_NONE) fade_in(4, white); else fade_set_level(0);
}
