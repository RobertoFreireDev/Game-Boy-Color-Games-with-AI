#ifndef UNIT_H
#define UNIT_H
/*
 * Tiny unit test framework for engine tests. Each test_<module>.c becomes its own
 * ROM (engine + font_main + unit.c + that file) and runs in tests/engine/gbc.mjs.
 * Results are printed on the serial port and read by tests/engine/run.mjs.
 *
 * A test file starts with `#pragma bank 255` (bank 0 is full with the engine) and defines:
 *   void unit_setup(void) BANKED { ... }        runs before every test
 *   void unit_tests(void) BANKED { RUN(a); RUN(b); }
 * main() (in unit.c) calls engine_init() once, then unit_tests().
 *
 * Banking: tests, their data and their callbacks live in the test file's bank, which
 * stays mapped while unit_tests() runs, so they can be passed to the engine with
 * bank 0 like unbanked assets. Everything the engine switches must be switched back.
 */
#include "engine/engine.h"

#define TEST(name) static void name(void)
#define RUN(name)  unit_run(name, #name)

#define ASSERT(cond) \
    do { if (!(cond)) { unit_fail(__LINE__, #cond, 0, 0, 0); return; } } while (0)
#define ASSERT_EQ(got, want) \
    do { int32_t g_ = (int32_t)(got), w_ = (int32_t)(want); \
         if (g_ != w_) { unit_fail(__LINE__, #got, g_, w_, 1); return; } } while (0)

void unit_setup(void) BANKED;
void unit_tests(void) BANKED;

void unit_run(void (*fn)(void), const char *name);
void unit_fail(uint16_t line, const char *expr, int32_t got, int32_t want, uint8_t has_values);
void unit_puts(const char *s);

/* back to a clean engine state (what the scene manager does between scenes) */
void unit_reset_engine(void);

/* ---- joypad mock: the engine's joypad() calls return these ---- */
extern uint16_t pad_reads;                              /* joypad() calls since the last pad_* call */
void pad_hold(uint8_t keys);                            /* every read returns keys */
void pad_script(const uint8_t *keys, uint8_t n);        /* one entry per read, then 0 forever */
void pad_loop(const uint8_t *keys, uint8_t n);          /* one entry per read, repeating */

/* ---- hardware readers ---- */
uint16_t hw_bkg_color(uint8_t pal, uint8_t col);        /* CGB palette RAM */
uint16_t hw_obj_color(uint8_t pal, uint8_t col);
uint8_t  hw_vram(uint8_t bank, uint16_t addr);
uint8_t  hw_bkg_tile(uint8_t x, uint8_t y);             /* BG map, bank 0 */
uint8_t  hw_bkg_attr(uint8_t x, uint8_t y);             /* BG map, bank 1 */
uint8_t  hw_win_tile(uint8_t x, uint8_t y);
uint8_t  hw_win_attr(uint8_t x, uint8_t y);
#endif
