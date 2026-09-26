/* src/engine/input.c + input.h: input_update, KEY_* macros, input_wait_press */
#pragma bank 255
#include "unit.h"

void unit_setup(void) BANKED { unit_reset_engine(); }

TEST(update_reads_joypad_once) {
    pad_hold(J_A | J_LEFT);
    input_update();
    ASSERT_EQ(keys, J_A | J_LEFT);
    ASSERT_EQ(pad_reads, 1);
}

TEST(update_keeps_previous_frame) {
    pad_hold(J_B);
    input_update();
    pad_hold(J_START);
    input_update();
    ASSERT_EQ(keys_prev, J_B);
    ASSERT_EQ(keys, J_START);
}

TEST(held_pressed_released_over_frames) {
    static const uint8_t s[] = { 0, J_A, J_A, 0 };
    pad_script(s, 4);
    input_update();
    ASSERT(!KEY_HELD(J_A)); ASSERT(!KEY_PRESSED(J_A)); ASSERT(!KEY_RELEASED(J_A));
    input_update();
    ASSERT(KEY_HELD(J_A));  ASSERT(KEY_PRESSED(J_A));  ASSERT(!KEY_RELEASED(J_A));
    input_update();
    ASSERT(KEY_HELD(J_A));  ASSERT(!KEY_PRESSED(J_A)); ASSERT(!KEY_RELEASED(J_A));
    input_update();
    ASSERT(!KEY_HELD(J_A)); ASSERT(!KEY_PRESSED(J_A)); ASSERT(KEY_RELEASED(J_A));
}

TEST(macros_only_look_at_their_keys) {
    keys = J_A | J_B; keys_prev = J_B | J_UP;
    ASSERT(KEY_PRESSED(J_A));
    ASSERT(!KEY_PRESSED(J_B));
    ASSERT(KEY_RELEASED(J_UP));
    ASSERT(!KEY_HELD(J_SELECT));
    ASSERT(!KEY_PRESSED(J_A | J_B));   /* a mask counts as pressed only if none of it was held */
    ASSERT(KEY_HELD(J_B | J_SELECT));
}

TEST(wait_press_returns_first_new_key_in_mask) {
    static const uint8_t s[] = { 0, J_B, 0, J_A | J_B };
    uint16_t t0;
    uint8_t k;
    pad_script(s, 4);
    t0 = sys_time;
    k = input_wait_press(J_A);
    ASSERT_EQ(k, J_A);
    ASSERT_EQ(pad_reads, 4);
    ASSERT((uint16_t)(sys_time - t0) >= 4);   /* one vsync per read */
    ASSERT_EQ(keys, J_A | J_B);
}

TEST(wait_press_ignores_key_already_held) {
    static const uint8_t s[] = { J_A, J_A, 0, J_A };
    keys = J_A;
    pad_script(s, 4);
    ASSERT_EQ(input_wait_press(J_A), J_A);
    ASSERT_EQ(pad_reads, 4);
}

TEST(wait_press_returns_every_new_key_in_mask) {
    static const uint8_t s[] = { 0, J_UP | J_START | J_B };
    pad_script(s, 2);
    ASSERT_EQ(input_wait_press(J_START | J_UP | J_DOWN), J_UP | J_START);
}

void unit_tests(void) BANKED {
    RUN(update_reads_joypad_once);
    RUN(update_keeps_previous_frame);
    RUN(held_pressed_released_over_frames);
    RUN(macros_only_look_at_their_keys);
    RUN(wait_press_returns_first_new_key_in_mask);
    RUN(wait_press_ignores_key_already_held);
    RUN(wait_press_returns_every_new_key_in_mask);
}
