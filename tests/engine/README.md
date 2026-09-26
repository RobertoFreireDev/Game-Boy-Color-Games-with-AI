# Engine unit tests

Unit tests for every file in `src/engine/`. They run the real engine code, built by GBDK exactly as the game is, inside a small headless Game Boy Color emulator written in Node. No npm packages are needed.

```
node tests/engine/run.mjs            # all suites
node tests/engine/run.mjs map text   # only test_map.c and test_text.c
node tests/engine/run.mjs -v         # also list the tests that pass
```

The runner needs GBDK (found through `GBDK_HOME`; the default is `C:\gbdk` on Windows and `~/gbdk` elsewhere, the same as `build.bat` / `build.sh`) and Node 18 or newer. ROMs and object files go to `build/tests/engine/`. The runner exits with code 1 if any test fails.

## Files

| File | What it is |
|---|---|
| `run.mjs` | Compiles the engine, `assets/fonts/font_main.c`, the support files and each `test_*.c`, links one ROM per suite, runs each ROM and prints the results |
| `gbc.mjs` | Headless CGB emulator: SM83 CPU, MBC5, VRAM/WRAM banks, CGB palettes, OAM DMA, HDMA, LY/STAT/VBlank timing, timer, double speed, serial port. It draws no pixels and plays no sound |
| `unit.h` / `unit.c` | Test framework: `TEST`, `RUN`, `ASSERT`, `ASSERT_EQ`, results over the serial port, a joypad mock, hardware readers and `main()` |
| `data_banked.c/.h` | Test assets pinned to ROM bank 2, used to check the engine's bank switching |
| `test_<module>.c` | One suite per engine file: `anim audio collide core fade gfx input map particles scene sprites text tiles tween` |

## How a suite runs

Each ROM contains the engine, `font_main.c`, `unit.c`, `data_banked.c` and one `test_<module>.c`. `main()` calls `engine_init()` once and then the suite's `unit_tests()`. `unit_setup()` runs before every test; most suites call `unit_reset_engine()` there, which does the same cleanup the scene manager does between scenes.

The tests check what the engine really writes: `shadow_OAM`, VRAM tiles and attributes (`hw_bkg_tile`, `hw_win_attr`, …), CGB palette RAM (`hw_bkg_color`, `hw_obj_color`), scroll and LCD registers, and the sound registers.

- **Input**: `unit.c` replaces GBDK's `joypad()`. Use `pad_hold(keys)`, `pad_script(seq, n)` (one entry per read, then 0) or `pad_loop(seq, n)` (repeats) to drive `input_update()` and the blocking functions (`input_wait_press`, `dialog_show`, `dialog_choice`, `menu_run`). `pad_reads` counts the reads.
- **Audio**: `test_audio.c` takes `audio_update` off the VBlank interrupt and calls it by hand, so one call is one frame. The emulator reads sound registers back exactly as they were written. Real hardware hides some bits, so these register checks only work in this emulator.
- **Hangs**: a suite that runs more than 30 emulated seconds (for example, a blocking call that never gets its key) is reported as hung, together with the last test that passed.

## Writing a test

```c
/* src/engine/foo.c: what this suite covers */
#pragma bank 255
#include "unit.h"

void unit_setup(void) BANKED { unit_reset_engine(); }

TEST(does_the_thing) {
    ASSERT_EQ(foo(2), 4);
    ASSERT(foo_ready());
}

void unit_tests(void) BANKED {
    RUN(does_the_thing);
}
```

Every test file starts with `#pragma bank 255`. Bank 0 is almost full with the engine and GBDK, so each suite is autobanked instead. Its code, `const` data and callbacks stay mapped while `unit_tests()` runs, so they can be passed to the engine with bank 0, the same way as unbanked game assets. A failed assertion prints its line, the expression and both values, then ends that test. The other tests still run.
