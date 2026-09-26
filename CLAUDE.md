# CLAUDE.md — Game Boy Color games with GBDK-2020 (C only)

You are the only developer on this project. The human describes games; you write **all** code, art, maps, music and sound effects as plain C files. Every asset format is designed to be written by hand, as text. The human may also paste PNG sprite sheets and maps into `source_art/`; you read them and convert them into the same C asset files (section 14). The build never reads a PNG.

This file holds the rules that apply to every task. The detailed references live in `docs/` and are read **on demand**: before a task, check the table below and read every file whose "Read when" matches. Follow their conventions exactly — the engine, the assets and the build all depend on them.

**Current state: template.** The engine (`src/engine/`), `src/main.c` and the shared UI assets (`assets/fonts/font_main.c`: font, dialog box tiles, `sfx_menu`) are finished and reusable. There is no game: `src/scenes/`, `src/game/` and every other `assets/` folder hold only a `file.txt` with a worked example of that folder's format, and `GAME.md` is an empty design template. Until a game defines `scene_title`, the build compiles everything and then fails at the link step with an undefined `_scene_title` — that is expected.

## Docs index (read on demand)

Section numbers are stable across files: "section 7.6" always means the heading `7.6` in the file listed here.

| File | Sections | Read when |
|---|---|---|
| [docs/engine-api.md](docs/engine-api.md) | 7.1–7.12 | Writing game code or scenes (core, input, gfx, sprites, anim, map/camera, collide, text/dialog/menus, scenes, fade, tween, particles), or changing `src/engine/` |
| [docs/graphics.md](docs/graphics.md) | 6, 6.1–6.6, 6.9, 8 | Writing palettes, sprites, tilesets, maps, font, `assets.h`; drawing any pixel art |
| [docs/audio.md](docs/audio.md) | 6.7, 6.8, 7.13, 11 | Writing music or SFX, or touching the audio driver |
| [docs/source-art.md](docs/source-art.md) | 14–14.4 | `source_art/` has PNGs to convert (new or replaced) |
| [docs/png2gb.md](docs/png2gb.md) | 14.5 | Actually measuring a PNG (the Node helper script) |
| [docs/banking.md](docs/banking.md) | 10 | Linker reports ROM/area overflow (past 32 KB) |
| [docs/debugging.md](docs/debugging.md) | 13 | A build fails, or something looks/sounds wrong in the emulator |
| [docs/setup.md](docs/setup.md) | 1.1, 1.2, 1.4, 12.1 | Installing tools, recreating build scripts / `.vscode` files, scaffolding a missing engine |

If you change something documented in a `docs/` file (engine API, asset format, build script), update that file in the same change.

---

## 0. Golden rules

1. **Language: C only** (SDCC through GBDK-2020's `lcc`). No C++, no asset converters in the build. Assets are `.c` files.
   For helper scripts during development (reading PNGs from `source_art/`, checking map row widths, counting music ticks, one-off calculations), use **Node.js** (`node -e "..."` or a throwaway `.js` in a temp folder), **never Python**. The build must never depend on them, and no `.js` files are committed to the project. The one exception is `tests/tools/` (tests for `tools/pixel-editor.html`, run with `node --test`): when you change the editor, update its tests and run them.
2. **Target: Game Boy Color only** (`-Wm-yC`). Always use CGB features: palettes, VRAM bank 1, BG attributes.
3. **Respect hardware limits** (section 4). If a design exceeds them, change the design, don't hope.
4. **No floats, no `malloc`, no recursion, no `printf` in game code.** Use integers and fixed point (section 9).
5. **Every `.c` file name in the project must be unique** (object files go to one folder).
6. **Never call `DISPLAY_OFF`** except inside `engine_init()` (the CGB screen flashes white). Load graphics while the screen is faded to black instead.
7. After any change, **build** (`build.bat` / `build.sh`) and fix every error and warning before finishing.
8. Keep engine code in `src/engine/`, game code in `src/scenes/` and `src/game/`, data in `assets/`, the human's source images in `source_art/`. Don't put game logic in assets or asset data in code.
9. The engine exists: build games on it and don't rewrite it. Change engine code only to fix a bug or add a reusable feature, and then update section 7 ([docs/engine-api.md](docs/engine-api.md)) to match. If `src/engine/` is ever missing, scaffold it (section 12.1, [docs/setup.md](docs/setup.md)).
10. **Never delete `assets/fonts/font_main.c`** and never define `sfx_menu` anywhere else: the engine's text, dialog and menu code needs `font_main`, `font_box_tiles` and `sfx_menu` from that file.
11. Keep the `file.txt` in every folder. It is the format example for the next game; it is not compiled (the build only picks up `.c` files).

---

## 1. Project layout

Install, build scripts and VS Code tasks: [docs/setup.md](docs/setup.md). Build with `build.bat` / `build.sh` → `build/game.gb`; run with `run.bat` (Emulicious).

### 1.3 Project structure

```
my-game/
├── CLAUDE.md                 ← this file (core rules + docs index)
├── docs/                     ← on-demand references (see Docs index)
├── GAME.md                   ← game design notes (AI keeps it updated)
├── build.bat  build.sh  run.bat
├── .vscode/
│   ├── tasks.json
│   └── c_cpp_properties.json
├── src/
│   ├── main.c                ← engine_init + main loop (tiny)
│   ├── engine/               ← reusable engine (section 7)
│   │   ├── engine.h          ← includes every engine header
│   │   ├── core.c/.h         ← init, types, fixed point, random
│   │   ├── tiles.h           ← PX() pixel macro
│   │   ├── input.c/.h
│   │   ├── gfx.c/.h          ← palettes, VRAM allocator, sprite loading
│   │   ├── sprites.c/.h      ← OAM allocator, metasprite drawing
│   │   ├── anim.c/.h
│   │   ├── map.c/.h          ← map loading, tile queries, camera, streaming
│   │   ├── collide.c/.h      ← AABB + tile collision, body_move
│   │   ├── text.c/.h         ← font, text drawing, dialog boxes, menus
│   │   ├── fade.c/.h         ← fade in/out (black/white)
│   │   ├── scene.c/.h        ← scene manager + transitions
│   │   ├── tween.c/.h
│   │   ├── particles.c/.h
│   │   └── audio.c/.h        ← music + sfx driver, note table, instruments
│   ├── scenes/               ← one file per scene: title.c, level.c, gameover.c
│   │   └── scenes.h
│   └── game/                 ← entities, player.c, enemies.c, game state
├── assets/
│   ├── assets.h              ← extern declarations for EVERY asset (keep updated)
│   ├── palettes/             ← pal_*.c
│   ├── sprites/              ← spr_*.c
│   ├── tilesets/             ← ts_*.c
│   ├── maps/                 ← map_*.c
│   ├── fonts/                ← font_main.c (font + box tiles + sfx_menu; shared by every game)
│   ├── music/                ← mus_*.c
│   └── sfx/                  ← sfx_all.c (or sfx_*.c)
├── tools/
│   └── pixel-editor.html     ← browser pixel editor for source_art PNGs (GBC palette rules built in); never built
├── tests/
│   └── tools/                ← tests for tools/ (`node --test`, headless Chrome/Edge, no npm); never built
└── source_art/               ← PNGs pasted by the human, converted by the AI (section 14); never built
    ├── spritesheets/         ← <name>.png (+ <name>.txt notes) → assets/sprites/spr_<name>.c
    └── mapsheets/            ← <name>.png (+ <name>.txt notes) → assets/tilesets/ts_*.c + assets/maps/map_<name>.c
```
Every game folder (`src/scenes/`, `src/game/`, each `assets/` subfolder, each `source_art/` subfolder) contains a `file.txt` with a complete example of what goes there. Read it before writing into that folder.

---

## 2. How to work (AI workflow)

For **a new game**: read the `file.txt` of each folder you'll write into → look in `source_art/` for PNGs and their notes → fill in `GAME.md` (genre, controls, scenes, entities, source art used, asset list, palette plan, VRAM budget) → convert the source art (section 14, [docs/source-art.md](docs/source-art.md)) and write the remaining assets by hand → write `src/game/` and `src/scenes/` (at least `scene_title`, added to `scenes.h`) → build → fix → summarize to the human what was made, how to play, and which images became which assets.

For **each change**: read `GAME.md` and the files involved → edit → update `assets/assets.h` and `GAME.md` → build → fix.

When the human adds or replaces a PNG in `source_art/`: convert it again (section 14), overwrite the generated asset file, keep tile/frame indices stable where game code or maps depend on them, then build.

To **start over with a new game**: delete the game's `.c`/`.h` files in `src/scenes/`, `src/game/` and `assets/` (except `assets/fonts/font_main.c`), reset `assets.h`, `scenes.h` and `GAME.md` to the template form, keep every `file.txt`. Delete old PNGs in `source_art/` only if the human asks.

When something can only be verified visually (art, feel, music), tell the human exactly what to look at in the emulator, and use `EMU_printf` (section 13, [docs/debugging.md](docs/debugging.md)) for debug output.

---

## 3. Game loop

**src/main.c** (always this shape):
```c
#include "engine/engine.h"
#include "scenes/scenes.h"

void main(void) {
    engine_init();                 /* CGB check, fast CPU, VRAM clear, font, audio, palettes black */
    scene_start(&scene_title);     /* enter() then fade in */
    while (1) {
        vsync();                   /* wait for VBlank: 60 frames per second */
        input_update();
        scene_update();            /* spr_begin(); cur->update(); spr_end(); + pending transitions */
    }
}
```

**A scene file** (`src/scenes/level.c`):
```c
#include "engine/engine.h"
#include "assets.h"
#include "scenes.h"

static sprite_t spr_hero;
static body_t hero;

static void level_enter(void) {
    map_load(&map_level1, 0);                  /* 0 = data not banked */
    gfx_load_sprite(&spr_hero, &spr_player, 0);
    uint16_t tx, ty;
    map_find('P', 0, &tx, &ty);
    hero.x = FIX(tx * 8); hero.y = FIX(ty * 8);
    hero.w = 12; hero.h = 14;
    cam_set(UNFIX(hero.x) - 80, UNFIX(hero.y) - 72);
    music_play(&mus_theme);
}

static void level_update(void) {
    /* 1. input  2. physics/logic  3. camera  4. draw sprites (every frame!) */
    if (KEY_PRESSED(J_START)) scene_goto(&scene_title, TRANS_FADE_BLACK);
    body_move(&hero);
    cam_follow(UNFIX(hero.x) + 6, UNFIX(hero.y) + 7);
    spr_draw(&spr_hero, 0, W2S_X(UNFIX(hero.x)), W2S_Y(UNFIX(hero.y)), 0);
    particles_update();
    tween_update();
}

const scene_t scene_level = { level_enter, level_update, 0 };
```

Sprites are **redrawn every frame** from game state (immediate mode). Anything not drawn this frame is hidden automatically by `spr_end()`.

---

## 4. Game Boy Color hardware limits (MUST respect)

| Resource | Limit |
|---|---|
| Screen | 160×144 px = 20×18 tiles |
| Tile | 8×8 px, 4 colors (2 bits per pixel), 16 bytes |
| BG tile map | 32×32 tiles (hardware wraps); larger maps are streamed by the engine |
| Colors | RGB555 (0–31 per channel). `RGB8(r,g,b)` accepts 0–255 and rounds |
| BG palettes | 8 × 4 colors. **One palette per 8×8 tile** (set per tile via attributes) |
| Sprite palettes | 8 × 4 colors, **color 0 is always transparent** → 3 visible colors per sprite tile |
| Sprites | **40 on screen, max 10 per scanline** (the 11th disappears). Engine uses 8×8 mode |
| Sprite size | Built from 8×8 tiles. A 16×16 character = 4 sprites, uses 2 of the 10 per line |
| VRAM | 2 banks × 384 tiles; engine layout in section 5 |
| Window | Second BG layer, not scrollable, used for dialog/HUD; x offset is +7 |
| CPU | ~8 MHz in CGB double speed (engine enables it). Still slow: no floats, avoid `*` `/` `%` in hot loops |
| WRAM | 8 KB usable by default; keep global RAM under ~6 KB. Non-`const` arrays live in RAM |
| ROM | 32 KB without banking; up to 8 MB with MBC5 banking (section 10) |
| Audio | 4 channels: CH1 pulse+sweep, CH2 pulse, CH3 wave, CH4 noise |

Design consequences:
- Keep enemies/bullets per horizontal line low. Prefer 8×8 or 8×16 bullets and 16×16 characters.
- Large bosses → draw them on the **background** (tiles), not with sprites.
- A single sprite tile can use only its one palette's 3 colors. For a 4th/5th color on a character, overlay a second sprite with another palette (costs sprites).
- A BG tile uses one palette. Plan tilesets so each tile needs ≤4 colors from one palette.

---

## 5. Engine conventions (VRAM and palettes)

### VRAM layout (tile indices, BG and sprites share tile data at 0x8000)

| Bank | Tiles | Use |
|---|---|---|
| 0 | 0–127 | Sprite tiles, filled by `gfx_load_sprite` (reset every scene) |
| 0 | 128–255 | Current map's tileset (max **128 BG tiles** per scene) |
| 1 | 0–127 | Free (advanced: extra sprite tiles with OAM bit 3) |
| 1 | 128–223 | Font, ASCII 32–127 (`tile = 128 + c - 32`) |
| 1 | 224–233 | Dialog box tiles (corners, edges, fill, "next" arrow) |
| 1 | 234–255 | Free (UI icons) |

Blank BG cell = font space (tile 128, bank 1, palette 7): `TILE_BLANK 128`, `ATTR_BLANK 0x0F`.

### Palette slots

- BG 0–6: tileset palettes (loaded by `map_load`). BG 7: **UI/text** (0 = box background, 1 = shadow, 2 = border, 3 = text).
- OBJ 0–7: sprite palettes, chosen per sprite asset (`pal_slot`). Suggested: 0 player, 1–5 enemies/items, 6 effects/particles, 7 UI cursor.

### Attribute bits (BG attribute byte and sprite prop byte share the layout)

| Bit | Meaning |
|---|---|
| 0–2 | Palette 0–7 |
| 3 | VRAM bank of the tile |
| 5 | Flip X (`S_FLIPX`) |
| 6 | Flip Y (`S_FLIPY`) |
| 7 | BG: tile drawn over sprites · Sprite: sprite behind BG colors 1–3 (`S_PRIORITY`) |

---

## 9. C coding rules for SDCC / GBDK

- Use `uint8_t`/`int8_t` whenever values fit; `int16_t`/`uint16_t` for world pixels and fixed point; `int` is 16-bit. Avoid `int32_t` except rare math.
- Never rely on plain `char` signedness; use explicit types. Map rows are `char` only as ASCII.
- All asset data `const` (goes to ROM). Non-`const` global arrays eat RAM.
- Prefer globals/`static` over large locals (small stack). No recursion. No variable-length arrays.
- Avoid `*`, `/`, `%` in per-frame loops; use shifts, precomputed tables, counters.
- Function pointers are fine (scenes, entity behaviors).
- Entities: fixed-size arrays of structs with an `active` flag; iterate with a `uint8_t` index. E.g. `#define MAX_ENEMIES 8`.
- Declare every function in a header; include `engine/engine.h` in all files.
- Game state that must survive scenes (score, lives, level) goes in `src/game/state.c`.
- Save data (optional): `ENABLE_RAM; SWITCH_RAM(0);` write a struct to `0xA000` with a magic number and checksum, then `DISABLE_RAM;`.

---

## 12. Checklists

### 12.0 New game from the template (normal case)
1. Read `GAME.md` (empty template) and the `file.txt` of every folder you will write into.
2. List `source_art/spritesheets/` and `source_art/mapsheets/`; read every PNG and its `.txt` notes.
3. Fill in `GAME.md`, including the "Source art" table (image → asset) and the per-scene VRAM budget.
4. Convert the source art ([docs/source-art.md](docs/source-art.md)); write the other assets by hand ([docs/graphics.md](docs/graphics.md), [docs/audio.md](docs/audio.md)).
5. `src/game/state.c` and other logic, then the scenes (at least `scene_title`), declared in `scenes.h`.
6. Update `assets.h`, build, fix, summarize.

### 12.2 Before building (verify, don't assume)
- [ ] Every `PX(...)` has exactly 8 values, each 0–3; every tile has exactly 8 rows.
- [ ] Sprite tile count = `w * h * frames`; tileset `count` = number of tiles written.
- [ ] Sprite visible pixels never use 0; each tile's colors fit one palette.
- [ ] Map rows: exactly `h` rows, each exactly `w` characters; every char has a legend entry; legend `pal` 0–6.
- [ ] Per scene: sprite tiles ≤ 128, BG tiles ≤ 128, ≤ 7 BG palettes (+ UI), ≤ 8 OBJ palettes.
- [ ] Worst case ≤ 10 sprite tiles on one scanline; ≤ 40 total.
- [ ] Music: each channel's loop length checked; SFX end with `SFX_END`.
- [ ] New assets declared in `assets.h`; new files have unique names.
- [ ] Every scene's `update` redraws all its sprites.
- [ ] Converted art: the helper's tile count and color-set count fit the limits; every generated asset names its source PNG in its first comment; `GAME.md` source-art table updated.
