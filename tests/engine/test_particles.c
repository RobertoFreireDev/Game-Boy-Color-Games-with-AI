/* src/engine/particles.c: emit, motion, gravity, lifetime, frame animation, pool of 12 */
#pragma bank 255
#include "unit.h"

static const sprite_t spr1 = { 20, 1, 1, 1, 1, 6 };   /* tile 20, OBJ palette 6 */
static const sprite_t spr3 = { 30, 1, 1, 1, 3, 6 };   /* 3 frames: tiles 30..32 */

static const particle_style_t st_still = { &spr1, 0, 0, 0, 10 };
static const particle_style_t st_grav  = { &spr1, 0, 0, 16, 20 };
static const particle_style_t st_up    = { &spr1, 0, -32, 0, 20 };
static const particle_style_t st_anim  = { &spr3, 0, 0, 0, 9 };
static const particle_style_t st_rand  = { &spr1, 32, 0, 0, 20 };

/* one game frame: returns the number of sprites drawn */
static uint8_t frame(void) {
    uint8_t i, n = 0;
    spr_begin();
    particles_update();
    spr_end();
    for (i = 0; i < 40; i++) if (shadow_OAM[i].y) n++;
    return n;
}

static uint8_t count_x(uint8_t x) {
    uint8_t i, n = 0;
    for (i = 0; i < 40; i++) if (shadow_OAM[i].y && shadow_OAM[i].x == x) n++;
    return n;
}

void unit_setup(void) BANKED { unit_reset_engine(); }

TEST(emit_draws_at_world_position) {
    particles_emit(50, 60, 1, &st_still);
    ASSERT_EQ(frame(), 1);
    ASSERT_EQ(shadow_OAM[0].x, 58);
    ASSERT_EQ(shadow_OAM[0].y, 76);
    ASSERT_EQ(shadow_OAM[0].tile, 20);
    ASSERT_EQ(shadow_OAM[0].prop, 6);
}

TEST(drawn_relative_to_camera) {
    cam_x = 30; cam_y = 10;
    particles_emit(50, 60, 1, &st_still);
    frame();
    ASSERT_EQ(shadow_OAM[0].x, 28);
    ASSERT_EQ(shadow_OAM[0].y, 66);
}

TEST(emit_count) {
    particles_emit(10, 10, 3, &st_still);
    ASSERT_EQ(frame(), 3);
}

TEST(nothing_drawn_before_emit) {
    ASSERT_EQ(frame(), 0);
}

TEST(lifetime_in_frames) {
    uint8_t i;
    particles_emit(10, 10, 1, &st_still);   /* life 10 */
    for (i = 0; i < 9; i++) ASSERT_EQ(frame(), 1);
    ASSERT_EQ(frame(), 0);
    ASSERT_EQ(frame(), 0);
}

TEST(gravity_accelerates) {
    particles_emit(50, 60, 1, &st_grav);     /* +1 px/frame each frame */
    frame(); ASSERT_EQ(shadow_OAM[0].y, 61 + 16);
    frame(); ASSERT_EQ(shadow_OAM[0].y, 63 + 16);
    frame(); ASSERT_EQ(shadow_OAM[0].y, 66 + 16);
    ASSERT_EQ(shadow_OAM[0].x, 58);
}

TEST(up_sets_initial_vertical_speed) {
    particles_emit(50, 60, 1, &st_up);       /* -2 px/frame */
    frame(); ASSERT_EQ(shadow_OAM[0].y, 58 + 16);
    frame(); ASSERT_EQ(shadow_OAM[0].y, 56 + 16);
}

TEST(frames_spread_over_lifetime) {
    particles_emit(50, 60, 1, &st_anim);     /* life 9, 3 frames: step every 3 */
    frame(); ASSERT_EQ(shadow_OAM[0].tile, 30);
    frame(); ASSERT_EQ(shadow_OAM[0].tile, 30);
    frame(); ASSERT_EQ(shadow_OAM[0].tile, 31);
    frame(); frame(); frame();
    ASSERT_EQ(shadow_OAM[0].tile, 32);
    frame(); frame();
    ASSERT_EQ(shadow_OAM[0].tile, 32);       /* stays on the last frame */
    ASSERT_EQ(frame(), 0);
}

TEST(pool_of_12_recycles_oldest) {
    particles_emit(10, 10, 12, &st_still);
    particles_emit(100, 10, 3, &st_still);
    ASSERT_EQ(frame(), 12);
    ASSERT_EQ(count_x(108), 3);
    ASSERT_EQ(count_x(18), 9);
}

TEST(random_speed_stays_in_range) {
    uint8_t i, spread = 0;
    int16_t dx, dy;
    initrand(1234);
    particles_emit(80, 70, 12, &st_rand);    /* speed 32 = up to 2 px/frame */
    ASSERT_EQ(frame(), 12);
    for (i = 0; i < 12; i++) {
        dx = (int16_t)shadow_OAM[i].x - 8 - 80;
        dy = (int16_t)shadow_OAM[i].y - 16 - 70;
        ASSERT(dx >= -2 && dx <= 2);
        ASSERT(dy >= -2 && dy <= 2);
        if (dx || dy) spread = 1;
    }
    ASSERT(spread);
}

TEST(offscreen_particles_live_on) {
    particles_emit(50, 60, 1, &st_still);
    cam_x = 500;
    ASSERT_EQ(frame(), 0);
    cam_x = 0;
    ASSERT_EQ(frame(), 1);
}

TEST(clear_removes_all_and_restarts_slots) {
    particles_emit(10, 10, 5, &st_still);
    particles_clear();
    ASSERT_EQ(frame(), 0);
    particles_emit(10, 10, 12, &st_still);
    ASSERT_EQ(frame(), 12);
}

void unit_tests(void) BANKED {
    RUN(emit_draws_at_world_position);
    RUN(drawn_relative_to_camera);
    RUN(emit_count);
    RUN(nothing_drawn_before_emit);
    RUN(lifetime_in_frames);
    RUN(gravity_accelerates);
    RUN(up_sets_initial_vertical_speed);
    RUN(frames_spread_over_lifetime);
    RUN(pool_of_12_recycles_oldest);
    RUN(random_speed_stays_in_range);
    RUN(offscreen_particles_live_on);
    RUN(clear_removes_all_and_restarts_slots);
}
