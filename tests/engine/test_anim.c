/* src/engine/anim.c: anim_play, anim_update, anim_frame */
#pragma bank 255
#include "unit.h"

static const uint8_t fr[] = { 4, 5, 6 };
static const anim_def_t walk = { fr, 3, 2, 1 };    /* loop, 2 frames per step */
static const anim_def_t once = { fr, 3, 1, 0 };    /* one shot, 1 frame per step */
static const anim_def_t idle = { fr, 1, 1, 1 };
static anim_t a;

static void update_n(uint8_t n) { while (n--) anim_update(&a); }

void unit_setup(void) BANKED { a.def = 0; a.i = a.timer = a.done = 0; }

TEST(play_starts_at_first_frame) {
    a.i = 2; a.timer = 1; a.done = 1;
    anim_play(&a, &walk);
    ASSERT(a.def == &walk);
    ASSERT_EQ(a.i, 0);
    ASSERT_EQ(a.timer, 0);
    ASSERT_EQ(a.done, 0);
    ASSERT_EQ(anim_frame(&a), 4);
}

TEST(play_same_def_does_not_restart) {
    anim_play(&a, &walk);
    update_n(2);
    ASSERT_EQ(a.i, 1);
    anim_play(&a, &walk);
    ASSERT_EQ(a.i, 1);
}

TEST(play_other_def_restarts) {
    anim_play(&a, &walk);
    update_n(3);
    anim_play(&a, &once);
    ASSERT_EQ(a.i, 0);
    ASSERT_EQ(a.timer, 0);
}

TEST(update_steps_every_speed_frames) {
    anim_play(&a, &walk);
    update_n(1); ASSERT_EQ(anim_frame(&a), 4);
    update_n(1); ASSERT_EQ(anim_frame(&a), 5);
    update_n(1); ASSERT_EQ(anim_frame(&a), 5);
    update_n(1); ASSERT_EQ(anim_frame(&a), 6);
}

TEST(looping_wraps_to_start) {
    anim_play(&a, &walk);
    update_n(6);
    ASSERT_EQ(a.i, 0);
    ASSERT_EQ(a.done, 0);
    update_n(2);
    ASSERT_EQ(a.i, 1);
}

TEST(one_shot_holds_last_frame_and_sets_done) {
    anim_play(&a, &once);
    update_n(2);
    ASSERT_EQ(a.i, 2);
    ASSERT_EQ(a.done, 0);
    update_n(1);
    ASSERT_EQ(a.done, 1);
    ASSERT_EQ(anim_frame(&a), 6);
    update_n(10);
    ASSERT_EQ(a.i, 2);
    ASSERT_EQ(a.timer, 0);                /* no longer counting */
}

TEST(finished_one_shot_restarts_only_with_other_def) {
    anim_play(&a, &once);
    update_n(3);
    anim_play(&a, &once);
    ASSERT_EQ(a.done, 1);
    anim_play(&a, &walk);
    ASSERT_EQ(a.done, 0);
}

TEST(single_frame_loop_stays) {
    anim_play(&a, &idle);
    update_n(5);
    ASSERT_EQ(a.i, 0);
    ASSERT_EQ(a.done, 0);
}

TEST(update_without_def_does_nothing) {
    update_n(3);
    ASSERT_EQ(a.timer, 0);
    ASSERT_EQ(a.i, 0);
}

void unit_tests(void) BANKED {
    RUN(play_starts_at_first_frame);
    RUN(play_same_def_does_not_restart);
    RUN(play_other_def_restarts);
    RUN(update_steps_every_speed_frames);
    RUN(looping_wraps_to_start);
    RUN(one_shot_holds_last_frame_and_sets_done);
    RUN(finished_one_shot_restarts_only_with_other_def);
    RUN(single_frame_loop_stays);
    RUN(update_without_def_does_nothing);
}
