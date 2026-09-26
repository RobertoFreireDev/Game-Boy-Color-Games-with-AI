# 15. Testing

The project has two unit test suites, both in `tests/` and both run with Node alone (no npm packages). Neither is part of the game build.

| Suite | Tests | Run from the project root | Needs |
|---|---|---|---|
| Engine (15.1) | every file in `src/engine/` | `node tests/engine/run.mjs` | GBDK-2020, Node 18+ |
| Tools (15.2) | `tools/pixel-editor.html` | `node --test "tests/tools/*.test.mjs"` | Chrome or Edge, Node 22+ |

**Rule:** every new feature, bug fix or behavior change in `src/engine/` or `tools/` comes with new or updated tests in the matching suite, in the same change. Run that suite and make it pass before finishing. If you change the test harness itself (runner, emulator, browser driver, `unit.h`), update this file.

---

## 15.1 Engine tests (`tests/engine/`)

They run the real engine code, built by GBDK exactly as the game is, inside a small headless Game Boy Color emulator written in Node.

```
node tests/engine/run.mjs            # all suites
node tests/engine/run.mjs map text   # only test_map.c and test_text.c
node tests/engine/run.mjs -v         # also list the tests that pass
```

The runner finds GBDK through `GBDK_HOME` (default `C:\gbdk` on Windows and `~/gbdk` elsewhere, the same as `build.bat` / `build.sh`). ROMs and object files go to `build/tests/engine/`. The runner exits with code 1 if any test fails.

### Files

| File | What it is |
|---|---|
| `run.mjs` | Compiles the engine, `assets/fonts/font_main.c`, the support files and each `test_*.c`, links one ROM per suite, runs each ROM and prints the results |
| `gbc.mjs` | Headless CGB emulator: SM83 CPU, MBC5, VRAM/WRAM banks, CGB palettes, OAM DMA, HDMA, LY/STAT/VBlank timing, timer, double speed, serial port. It draws no pixels and plays no sound |
| `unit.h` / `unit.c` | Test framework: `TEST`, `RUN`, `ASSERT`, `ASSERT_EQ`, results over the serial port, a joypad mock, hardware readers and `main()` |
| `data_banked.c/.h` | Test assets pinned to ROM bank 2, used to check the engine's bank switching |
| `test_<module>.c` | One suite per engine file: `anim audio collide core fade gfx input map particles scene sprites text tiles tween` |

A new engine file `src/engine/foo.c` gets a new `tests/engine/test_foo.c`; the runner picks up every `test_*.c` by itself.

### How a suite runs

Each ROM contains the engine, `font_main.c`, `unit.c`, `data_banked.c` and one `test_<module>.c`. `main()` calls `engine_init()` once and then the suite's `unit_tests()`. `unit_setup()` runs before every test; most suites call `unit_reset_engine()` there, which does the same cleanup the scene manager does between scenes.

The tests check what the engine really writes: `shadow_OAM`, VRAM tiles and attributes (`hw_bkg_tile`, `hw_win_attr`, …), CGB palette RAM (`hw_bkg_color`, `hw_obj_color`), scroll and LCD registers, and the sound registers.

- **Input**: `unit.c` replaces GBDK's `joypad()`. Use `pad_hold(keys)`, `pad_script(seq, n)` (one entry per read, then 0) or `pad_loop(seq, n)` (repeats) to drive `input_update()` and the blocking functions (`input_wait_press`, `dialog_show`, `dialog_choice`, `menu_run`). `pad_reads` counts the reads.
- **Audio**: `test_audio.c` takes `audio_update` off the VBlank interrupt and calls it by hand, so one call is one frame. The emulator reads sound registers back exactly as they were written. Real hardware hides some bits, so these register checks only work in this emulator.
- **Hangs**: a suite that runs more than 30 emulated seconds (for example, a blocking call that never gets its key) is reported as hung, together with the last test that passed.

### Writing a test

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

---

## 15.2 Tool tests (`tests/tools/`)

They load `tools/pixel-editor.html` in a headless Chrome or Edge and test it through the editor's own functions and through real mouse and keyboard input.

```
node --test "tests/tools/*.test.mjs"                               # all tests
node --test --test-name-pattern="undo" "tests/tools/*.test.mjs"    # only tests whose name matches
```

The browser is found in the usual Chrome/Edge install paths on Windows, macOS and Linux; set `CHROME_PATH` to use another Chromium-based browser.

### Files

| File | What it is |
|---|---|
| `browser.mjs` | Tiny DevTools-protocol driver: `launch()` starts the browser and returns a `page` with `eval`, `run(fn, ...args)`, `goto`, `waitFor`, `mouse(type, x, y, opts)` (trusted pointer events) and `errors` (uncaught page exceptions) |
| `pixel-editor.test.mjs` | The editor tests (`node:test` + `node:assert/strict`), grouped with `describe` by feature: startup, colors, palettes, image buffer, undo/redo, GBC tile analysis, Check panel, color editor, view, geometry, drawing, selection & clipboard, transforms, canvas size, mouse drawing, keyboard shortcuts, tool bar, new image, import, export, autosave, animation preview |

### How the tests run

One browser is launched for the whole file. Before every test the editor is reloaded with empty `localStorage` (set `sessionStorage.keepStorage` before a reload to keep it, as the autosave tests do), and `installHelpers()` adds a `T` helper object to the page: `T.doc(w, h, mode)` starts a fresh image, `T.px`/`T.rect`/`T.count` read and paint pixels, `T.key` sends key events, `T.screen(x, y)` converts image pixels to screen coordinates, `T.png`/`T.file` build test PNGs, `T.stubSave()` captures exports, `T.toasts` records messages.

The test file wraps these as `run(fn, ...args)` (run a function inside the page; arguments must be JSON), `click(x, y)`, `drag(points)` and `hover(x, y)` in image-pixel coordinates.

### Writing a test

Add it to the `describe` block of the feature it covers (or a new block for a new feature):

```js
describe('transforms', () => {
  test('flipH mirrors the whole image', async () => {
    const r = await run(() => { T.doc(16, 8); putPx(0, 0, 7); applyTransform('flipH'); return T.px(15, 0); });
    assert.equal(r, 7);
  });
});
```

Prefer asserting on the editor's state (`S`, the DOM) over pixels on screen. When a feature is driven by the mouse or keyboard, add at least one test that uses `click`/`drag`/`T.key`, not only the underlying function.
