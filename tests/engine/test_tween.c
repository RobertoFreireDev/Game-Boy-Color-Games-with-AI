/* src/engine/tween.c: tween_start, tween_update, easing, pool, tween_busy, tween_clear */
#pragma bank 255
#include "unit.h"

static int16_t v, w, pool_vals[9];

static void update_n(uint8_t n) { while (n--) tween_update(); }

void unit_setup(void) BANKED {
    tween_clear();
    v = w = 0;
}

TEST(start_marks_busy_without_moving) {
    ASSERT_EQ(tween_start(&v, 160, 16, EASE_LINEAR), 1);
    ASSERT(tween_busy(&v));
    ASSERT(!tween_busy(&w));
    ASSERT_EQ(v, 0);
}

TEST(linear_progress_and_end) {
    tween_start(&v, 160, 16, EASE_LINEAR);
    update_n(8);
    ASSERT_EQ(v, 80);
    update_n(7);
    ASSERT_EQ(v, 150);
    ASSERT(tween_busy(&v));
    update_n(1);
    ASSERT_EQ(v, 160);
    ASSERT(!tween_busy(&v));
    v = 5;
    update_n(3);
    ASSERT_EQ(v, 5);                       /* finished tween no longer writes */
}

TEST(ease_curves_at_half_and_quarter) {
    tween_start(&v, 160, 16, EASE_IN);
    update_n(4); ASSERT_EQ(v, 10);
    update_n(4); ASSERT_EQ(v, 40);
    v = 0; tween_clear(); tween_start(&v, 160, 16, EASE_OUT);
    update_n(8); ASSERT_EQ(v, 120);
    v = 0; tween_clear(); tween_start(&v, 160, 16, EASE_INOUT);
    update_n(4); ASSERT_EQ(v, 20);
    update_n(4); ASSERT_EQ(v, 80);
}

TEST(interpolates_between_table_entries) {
    tween_start(&v, 256, 3, EASE_LINEAR);
    update_n(1); ASSERT_EQ(v, 85);
    update_n(1); ASSERT_EQ(v, 170);
    update_n(1); ASSERT_EQ(v, 256);
}

TEST(negative_direction) {
    v = 100;
    tween_start(&v, -100, 4, EASE_LINEAR);
    update_n(1); ASSERT_EQ(v, 50);
    update_n(1); ASSERT_EQ(v, 0);
    update_n(2); ASSERT_EQ(v, -100);
}

TEST(large_range_uses_32_bit_math) {
    v = -2000;
    tween_start(&v, 2000, 16, EASE_LINEAR);
    update_n(8);
    ASSERT_EQ(v, 0);
    update_n(4);
    ASSERT_EQ(v, 1000);
}

TEST(zero_frames_finish_on_first_update) {
    tween_start(&v, 7, 0, EASE_LINEAR);
    update_n(1);
    ASSERT_EQ(v, 7);
    ASSERT(!tween_busy(&v));
}

TEST(restart_same_target_reuses_slot_from_current_value) {
    uint8_t i;
    tween_start(&v, 160, 16, EASE_LINEAR);
    update_n(8);
    ASSERT_EQ(tween_start(&v, 0, 4, EASE_LINEAR), 1);
    update_n(2);
    ASSERT_EQ(v, 40);                      /* from 80 to 0, halfway */
    for (i = 0; i < 7; i++) ASSERT_EQ(tween_start(&pool_vals[i], 1, 2, 0), 1);   /* 7 slots still free */
}

TEST(pool_holds_8) {
    uint8_t i;
    for (i = 0; i < 8; i++) ASSERT_EQ(tween_start(&pool_vals[i], 10, 4, 0), 1);
    ASSERT_EQ(tween_start(&pool_vals[8], 10, 4, 0), 0);
    ASSERT(!tween_busy(&pool_vals[8]));
    ASSERT_EQ(tween_start(&pool_vals[3], 99, 1, 0), 1);   /* same target replaces even when full */
    update_n(1);
    ASSERT_EQ(pool_vals[3], 99);
    ASSERT_EQ(tween_start(&pool_vals[8], 10, 4, 0), 1);   /* its slot came free */
}

TEST(concurrent_tweens_are_independent) {
    tween_start(&v, 100, 4, EASE_LINEAR);
    tween_start(&w, -40, 2, EASE_LINEAR);
    update_n(1);
    ASSERT_EQ(v, 25);
    ASSERT_EQ(w, -20);
    update_n(1);
    ASSERT_EQ(v, 50);
    ASSERT_EQ(w, -40);
    ASSERT(!tween_busy(&w));
    ASSERT(tween_busy(&v));
}

TEST(clear_stops_everything) {
    tween_start(&v, 100, 4, EASE_LINEAR);
    tween_start(&w, 100, 4, EASE_LINEAR);
    update_n(1);
    tween_clear();
    ASSERT(!tween_busy(&v));
    ASSERT(!tween_busy(&w));
    update_n(5);
    ASSERT_EQ(v, 25);
    ASSERT_EQ(w, 25);
}

TEST(ease_is_masked_to_2_bits) {
    tween_start(&v, 160, 16, 5);           /* 5 & 3 = EASE_IN */
    update_n(8);
    ASSERT_EQ(v, 40);
}

void unit_tests(void) BANKED {
    RUN(start_marks_busy_without_moving);
    RUN(linear_progress_and_end);
    RUN(ease_curves_at_half_and_quarter);
    RUN(interpolates_between_table_entries);
    RUN(negative_direction);
    RUN(large_range_uses_32_bit_math);
    RUN(zero_frames_finish_on_first_update);
    RUN(restart_same_target_reuses_slot_from_current_value);
    RUN(pool_holds_8);
    RUN(concurrent_tweens_are_independent);
    RUN(clear_stops_everything);
    RUN(ease_is_masked_to_2_bits);
}
