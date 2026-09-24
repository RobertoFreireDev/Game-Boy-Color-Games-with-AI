# CLAUDE.md — Game Boy Color games with GBDK-2020 (C only)

You are the only developer on this project. The human describes games; you write **all** code, art, maps, music and sound effects as plain C files. Nothing is drawn in external editors, so every asset format below is designed to be written by hand, as text.

Read this whole file before any task. Follow its conventions exactly — the engine, the assets and the build all depend on them.

---

## 0. Golden rules

1. **Language: C only** (SDCC through GBDK-2020's `lcc`). No C++, no Python/JS tools, no asset converters. Assets are `.c` files.
2. **Target: Game Boy Color only** (`-Wm-yC`). Always use CGB features: palettes, VRAM bank 1, BG attributes.
3. **Respect hardware limits** (section 4). If a design exceeds them, change the design, don't hope.
4. **No floats, no `malloc`, no recursion, no `printf` in game code.** Use integers and fixed point (section 9).
5. **Every `.c` file name in the project must be unique** (object files go to one folder).
6. **Never call `DISPLAY_OFF`** except inside `engine_init()` (the CGB screen flashes white). Load graphics while the screen is faded to black instead.
7. After any change, **build** (`build.bat` / `build.sh`) and fix every error and warning before finishing.
8. Keep engine code in `src/engine/`, game code in `src/scenes/` and `src/game/`, data in `assets/`. Don't put game logic in assets or asset data in code.
9. If `src/engine/` does not exist yet, scaffold it first (section 12.1), then build an empty title scene to prove the toolchain works.

---

## 1. Setup and tutorial (for the human)

### 1.1 Install (Windows)

1. **GBDK-2020**: download the latest `gbdk-win64.zip` from https://github.com/gbdk-2020/gbdk-2020/releases and extract it to `C:\gbdk` (so `C:\gbdk\bin\lcc.exe` exists). Then run once in a terminal:
   `setx GBDK_HOME C:\gbdk`
2. **Emulicious** (accurate emulator with tile/palette/memory viewers): download from https://emulicious.net, extract to `C:\Emulicious`, then:
   `setx EMULICIOUS C:\Emulicious\Emulicious.exe`
   (Emulicious needs Java; the site explains which download includes it.)
3. **Visual Studio Code** extensions: *C/C++* (`ms-vscode.cpptools`). Optional: *Emulicious Debugger* for stepping through C code (see its README; build with `-debug`).
4. Restart VS Code so it sees the new environment variables.

Linux/macOS: extract GBDK anywhere, `export GBDK_HOME=/path/to/gbdk`, and use `build.sh`.

### 1.2 Build and run

- **Build**: `Ctrl+Shift+B` in VS Code, or run `build.bat` in the project folder. Output: `build/game.gb`.
- **Run**: `Terminal → Run Task → Run` (builds, then opens Emulicious), or run `run.bat`.
- You can also drag `build/game.gb` onto any GBC emulator or flash it to a cartridge.

The ROM keeps the `.gb` extension; the header marks it as Color-only, which is what emulators check.

### 1.3 Project structure

```
my-game/
├── CLAUDE.md                 ← this file
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
└── assets/
    ├── assets.h              ← extern declarations for EVERY asset (keep updated)
    ├── palettes/             ← pal_*.c
    ├── sprites/              ← spr_*.c
    ├── tilesets/             ← ts_*.c
    ├── maps/                 ← map_*.c
    ├── fonts/                ← font_main.c
    ├── music/                ← mus_*.c
    └── sfx/                  ← sfx_all.c (or sfx_*.c)
```

### 1.4 Build scripts (create exactly these)

**build.bat**
```bat
@echo off
setlocal enabledelayedexpansion
if "%GBDK_HOME%"=="" set "GBDK_HOME=C:\gbdk"
set "LCC=%GBDK_HOME%\bin\lcc.exe"
set "CFLAGS=-Isrc -Iassets"
set "LFLAGS=-Wm-yC -Wm-ynGAME -Wl-yt0x1B -Wl-ya1 -Wl-yoA -autobank"
if exist build rmdir /s /q build
mkdir build\obj
set "OBJS="
for /r src %%f in (*.c) do (
  "%LCC%" %CFLAGS% -c -o "build\obj\%%~nf.o" "%%f" || goto :fail
  set "OBJS=!OBJS! build\obj\%%~nf.o"
)
for /r assets %%f in (*.c) do (
  "%LCC%" %CFLAGS% -c -o "build\obj\%%~nf.o" "%%f" || goto :fail
  set "OBJS=!OBJS! build\obj\%%~nf.o"
)
"%LCC%" %LFLAGS% -o build\game.gb !OBJS! || goto :fail
echo BUILD OK: build\game.gb
exit /b 0
:fail
echo BUILD FAILED
exit /b 1
```

**build.sh**
```sh
#!/bin/sh
set -e
LCC="${GBDK_HOME:-$HOME/gbdk}/bin/lcc"
rm -rf build && mkdir -p build/obj
OBJS=""
for f in $(find src assets -name '*.c'); do
  o="build/obj/$(basename "${f%.c}").o"
  "$LCC" -Isrc -Iassets -c -o "$o" "$f"
  OBJS="$OBJS $o"
done
"$LCC" -Wm-yC -Wm-ynGAME -Wl-yt0x1B -Wl-ya1 -Wl-yoA -autobank -o build/game.gb $OBJS
echo "BUILD OK: build/game.gb"
```

**run.bat**
```bat
@echo off
if "%EMULICIOUS%"=="" set "EMULICIOUS=C:\Emulicious\Emulicious.exe"
start "" "%EMULICIOUS%" "%~dp0build\game.gb"
```

Flags: `-Wm-yC` Color-only · `-Wm-ynGAME` header title (max 11 chars, A–Z) · `-Wl-yt0x1B` MBC5+RAM+battery · `-Wl-ya1` one 8 KB save-RAM bank · `-Wl-yoA` automatic ROM size · `-autobank` place `#pragma bank 255` files automatically.

**.vscode/tasks.json**
```json
{
  "version": "2.0.0",
  "tasks": [
    { "label": "Build", "type": "shell",
      "command": "${workspaceFolder}/build.bat",
      "linux": { "command": "./build.sh" }, "osx": { "command": "./build.sh" },
      "group": { "kind": "build", "isDefault": true }, "problemMatcher": [] },
    { "label": "Run", "type": "shell",
      "command": "${workspaceFolder}/run.bat",
      "linux": { "command": "emulicious build/game.gb" },
      "osx": { "command": "emulicious build/game.gb" },
      "dependsOn": "Build", "problemMatcher": [] }
  ]
}
```

**.vscode/c_cpp_properties.json** (IntelliSense only; the real compiler is `lcc`)
```json
{
  "configurations": [{
    "name": "GBDK",
    "includePath": ["${env:GBDK_HOME}/include", "${workspaceFolder}/src", "${workspaceFolder}/assets"],
    "defines": ["__PORT_sm83", "__TARGET_gb", "__SDCC", "NONBANKED=", "BANKED=", "CRITICAL=",
                "__critical=", "__banked=", "__nonbanked=", "__at(x)=", "__sfr=", "__naked=",
                "__interrupt=", "__reentrant="],
    "cStandard": "c99", "intelliSenseMode": "gcc-x86"
  }],
  "version": 4
}
```

Add `build/` to `.gitignore`.

---

## 2. How to work (AI workflow)

For **a new game**: write `GAME.md` (genre, controls, scenes, entities, asset list, palette plan, VRAM budget) → scaffold engine if missing → create assets → create scenes → build → fix → summarize to the human what was made and how to play.

For **each change**: read `GAME.md` and the files involved → edit → update `assets/assets.h` and `GAME.md` → build → fix.

When something can only be verified visually (art, feel, music), tell the human exactly what to look at in the emulator, and use `EMU_printf` (section 13) for debug output.

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

## 6. Asset formats (all plain C)

Every asset file:
- starts with `#include "engine/engine.h"` (so types/macros are visible),
- contains only `const` data (ROM), never code,
- has a matching `extern` line in `assets/assets.h`,
- has a unique file name with a type prefix: `pal_`, `spr_`, `ts_`, `map_`, `font_`, `mus_`, `sfx_`.

### 6.1 Pixels: the `PX` macro (src/engine/tiles.h)

One `PX(...)` = one row of 8 pixels, each value **0–3** (palette color index). 8 rows = one tile.

```c
#ifndef TILES_H
#define TILES_H
#include <stdint.h>
#define PXL_(a,b,c,d,e,f,g,h) ((uint8_t)((((a)&1)<<7)|(((b)&1)<<6)|(((c)&1)<<5)|(((d)&1)<<4)| \
                                          (((e)&1)<<3)|(((f)&1)<<2)|(((g)&1)<<1)|((h)&1)))
#define PXH_(a,b,c,d,e,f,g,h) PXL_((a)>>1,(b)>>1,(c)>>1,(d)>>1,(e)>>1,(f)>>1,(g)>>1,(h)>>1)
/* Game Boy 2bpp row = low bit-plane byte, then high bit-plane byte */
#define PX(a,b,c,d,e,f,g,h) PXL_(a,b,c,d,e,f,g,h), PXH_(a,b,c,d,e,f,g,h)
#define TILE_BYTES 16
#endif
```

### 6.2 Palette — `assets/palettes/pal_forest.c`

```c
#include "engine/engine.h"
/* index:            0 (sprite: transparent)  1 dark         2 mid            3 light */
const palette_color_t pal_forest_ground[4] = { RGB8(232,240,200), RGB8(136,176,80), RGB8(56,104,48), RGB8(16,40,24) };
const palette_color_t pal_forest_water[4]  = { RGB8(200,232,248), RGB8(104,168,224), RGB8(40,96,176), RGB8(16,40,88) };
```
Convention: for BG, 0 = lightest/background; for sprites, 0 = transparent, 1 = outline (darkest), 2 = main, 3 = highlight. Palettes may also live inside the sprite/tileset file that uses them.

### 6.3 Sprite — `assets/sprites/spr_player.c`

```c
typedef struct {                /* engine type, in gfx.h */
    const uint8_t *tiles;       /* 2bpp data: frame 0 tiles, then frame 1 tiles, ... */
    uint8_t w, h;               /* frame size in 8x8 tiles */
    uint8_t frames;
    const palette_color_t *pal; /* 4 colors, color 0 transparent */
    uint8_t pal_slot;           /* OBJ palette 0-7 */
} sprite_def_t;
```
Tile order inside a frame: **row-major** (top-left, top-right, bottom-left, bottom-right for 2×2).

```c
#include "engine/engine.h"

const palette_color_t pal_player[4] = { RGB8(0,0,0), RGB8(24,16,40), RGB8(216,72,56), RGB8(248,208,160) };

/* 16x16, 2 frames (idle, step). 0 = transparent, 1 = outline, 2 = clothes, 3 = skin */
const uint8_t spr_player_tiles[] = {
    /* frame 0, tile 0 (top-left) */
    PX(0,0,0,0,0,1,1,1),
    PX(0,0,0,0,1,2,2,2),
    PX(0,0,0,1,2,2,2,2),
    PX(0,0,0,1,3,3,1,3),
    PX(0,0,0,1,3,3,1,3),
    PX(0,0,0,1,3,3,3,3),
    PX(0,0,0,0,1,3,3,3),
    PX(0,0,0,1,2,1,1,1),
    /* frame 0, tile 1 (top-right) */
    /* ... 8 rows ... */
    /* frame 0, tile 2 (bottom-left), tile 3 (bottom-right) */
    /* frame 1, tiles 0..3 */
};

const sprite_def_t spr_player = { spr_player_tiles, 2, 2, 2, pal_player, 0 };
```
Animations are **not** stored here; they are tiny frame lists in game code (section 7.5).

### 6.4 Tileset — `assets/tilesets/ts_forest.c`

```c
typedef struct {
    const uint8_t *tiles; uint8_t count;              /* count <= 128 */
    const palette_color_t * const *pals; uint8_t pal_count;  /* loaded into BG slots 0..pal_count-1 (max 7) */
} tileset_def_t;
```
```c
#include "engine/engine.h"
extern const palette_color_t pal_forest_ground[4], pal_forest_water[4];

const uint8_t ts_forest_tiles[] = {
    /* 0: grass */
    PX(0,0,0,0,0,0,0,0), PX(0,0,1,0,0,0,0,0), PX(0,0,0,0,0,0,0,0), PX(0,0,0,0,0,1,0,0),
    PX(0,0,0,0,0,0,0,0), PX(0,1,0,0,0,0,0,0), PX(0,0,0,0,0,0,1,0), PX(0,0,0,0,0,0,0,0),
    /* 1: stone block */
    PX(1,1,1,1,1,1,1,3), PX(1,2,2,2,2,2,2,3), PX(1,2,2,2,2,2,2,3), PX(1,2,2,2,2,2,2,3),
    PX(1,2,2,2,2,2,2,3), PX(1,2,2,2,2,2,2,3), PX(1,2,2,2,2,2,2,3), PX(3,3,3,3,3,3,3,3),
    /* 2: water ... */
};
const palette_color_t * const ts_forest_pals[] = { pal_forest_ground, pal_forest_water };
const tileset_def_t ts_forest = { ts_forest_tiles, 3, ts_forest_pals, 2 };
```
Comment every tile with its index and name — maps refer to tiles by index.

### 6.5 Map — `assets/maps/map_level1.c`

Maps are **ASCII art**. A legend maps each character to tile + palette + flags.

```c
typedef struct { char ch; uint8_t tile; uint8_t pal; uint8_t flags; } map_legend_t; /* list ends with ch 0 */
typedef struct {
    uint8_t w, h;                      /* in tiles, 20..200 each */
    const char * const *rows;          /* h strings, each exactly w chars */
    const map_legend_t *legend;
    const tileset_def_t *tileset;
} map_def_t;

/* tile flags */
#define TF_SOLID    0x01   /* blocks movement */
#define TF_HAZARD   0x02   /* hurts */
#define TF_ONEWAY   0x04   /* platform solid only from above */
#define TF_TRIGGER  0x08   /* game-defined (door, sign, exit) */
#define TF_SPAWN    0x10   /* object marker: drawn as its tile, found with map_find() */
#define TF_FLIPX    0x20   /* visual, copied to attribute */
#define TF_FLIPY    0x40
#define TF_OVER     0x80   /* BG tile drawn over sprites */
```
```c
#include "engine/engine.h"
extern const tileset_def_t ts_forest;

static const char * const rows[] = {
 /*          1111111111222222222233333333334 */
 /*01234567890123456789012345678901234567890 */
  "########################################",
  "#......................................#",
  "#..............~~~~....................#",
  "#..P...........~~~~.........E......X...#",
  "########################################",
};
static const map_legend_t legend[] = {
    { '.', 0, 0, 0 },
    { '#', 1, 0, TF_SOLID },
    { '~', 2, 1, TF_HAZARD },
    { 'P', 0, 0, TF_SPAWN },      /* player start (drawn as grass) */
    { 'E', 0, 0, TF_SPAWN },      /* enemy */
    { 'X', 0, 0, TF_SPAWN | TF_TRIGGER },  /* exit */
    { 0, 0, 0, 0 }
};
const map_def_t map_level1 = { 40, 5, rows, legend, &ts_forest };
```
Rules: every row exactly `w` characters; every character used has a legend entry; `pal` is 0–6; legend `tile` < tileset count. Title screens, menus and HUD backgrounds are also maps (20×18).

### 6.6 Font — `assets/fonts/font_main.c`

1 bit per pixel, 8 bytes per glyph, 96 glyphs (ASCII 32–127), glyph drawn in the top-left 7×7 (leave right column and bottom row empty for spacing). Bit 1 = text color (UI palette color 3), bit 0 = background (color 0).

```c
#include "engine/engine.h"
const uint8_t font_main[96 * 8] = {
    /* ' ' */ 0,0,0,0,0,0,0,0,
    /* '!' */ 0b00110000,0b00110000,0b00110000,0b00110000,0b00000000,0b00110000,0b00000000,0b00000000,
    /* ... all 96 glyphs, in ASCII order ... */
};
/* 10 dialog-box tiles, 2bpp with PX(): TL, T, TR, L, FILL, R, BL, B, BR, NEXT-ARROW */
const uint8_t font_box_tiles[10 * 16] = { /* PX rows */ };
```

### 6.7 Music — `assets/music/mus_theme.c`

Each channel is a byte stream of commands. Time unit = **tick** = one 16th note. Song tempo = frames per tick (`fpt`).

```c
typedef struct { uint8_t fpt; const uint8_t *ch[4]; } song_t;   /* ch[i] may be 0 (unused) */

/* commands (audio.h) */
#define N(note,len)   NOTE_##note, (len)      /* play note on CH1/CH2/CH3 */
#define D(drum,len)   DRUM_##drum, (len)      /* play drum on CH4 */
#define R(len)        0xF0, (len)             /* rest (silence) */
#define INST(i)       0xF1, (i)               /* change instrument */
#define MUS_LOOP_POINT 0xFD                   /* loop returns here (default: start) */
#define MUS_LOOP       0xFE                   /* jump to loop point */
#define MUS_END        0xFF                   /* stop channel */

/* lengths in ticks */
#define L1 16
#define L2D 12
#define L2 8
#define L4D 6
#define L4 4
#define L8D 3
#define L8 2
#define L16 1
```
Notes: `C2 Cs2 D2 Ds2 E2 F2 Fs2 G2 Gs2 A2 As2 B2` … up to `B7` (`s` = sharp). Use flats as the sharp below (Bb4 = `As4`). CH3 plays at written pitch (engine compensates).

Tempo: `BPM ≈ 900 / fpt` → fpt 5 = 180 BPM, 6 = 150, 7 = 128, 8 = 112, 10 = 90.

```c
#include "engine/engine.h"

static const uint8_t ch1[] = {           /* harmony (SFX may interrupt this channel) */
    INST(INST_SOFT), MUS_LOOP_POINT,
    N(G4,L4), N(D5,L4), N(C5,L4), N(B4,L4),
    N(A4,L4), N(G4,L4), N(B4,L2),
    MUS_LOOP
};
static const uint8_t ch2[] = {           /* melody */
    INST(INST_LEAD), MUS_LOOP_POINT,
    N(E5,L8), N(G5,L8), N(C6,L4), N(B5,L8), N(G5,L8), N(A5,L4),
    N(G5,L8), N(E5,L8), N(F5,L8), N(D5,L8), N(C5,L2),
    MUS_LOOP
};
static const uint8_t ch3[] = {           /* bass */
    INST(WAVE_TRI), MUS_LOOP_POINT,
    N(C3,L4), N(G3,L4), N(A3,L4), N(E3,L4),
    N(F3,L4), N(C3,L4), N(G3,L2),
    MUS_LOOP
};
static const uint8_t ch4[] = {           /* drums */
    MUS_LOOP_POINT,
    D(KICK,L8), D(HAT,L8), D(SNARE,L8), D(HAT,L8),
    MUS_LOOP
};
const song_t mus_theme = { 7, { ch1, ch2, ch3, ch4 } };
```
**Rule:** the looped part of every channel must have the same total length in ticks, or a length that divides it (here 32, 32, 32, 8). Always count ticks in a comment per bar.

Instruments (engine table, pick by name):
- CH1/CH2 pulse: `INST_LEAD` (50% duty, medium decay), `INST_SQUARE` (50%, sustained), `INST_THIN` (12.5%, bright), `INST_PLUCK` (25%, short), `INST_SOFT` (25%, quiet), `INST_ECHO` (50%, very quiet — use for fake echo a 16th behind the melody).
- CH3 wave: `WAVE_TRI` (round bass), `WAVE_SAW` (buzzy), `WAVE_SQUARE` (hollow), `WAVE_SOFTTRI` (quiet).
- CH4 drums: `KICK`, `SNARE`, `HAT`, `OHAT`, `CRASH`, `TOM`.

### 6.8 Sound effects — `assets/sfx/sfx_all.c`

An SFX takes over **CH1** (tones, has pitch sweep) or **CH4** (noise) for a few frames; music on that channel is muted meanwhile and resumes at its next note.

```c
typedef struct { uint8_t ch; uint8_t prio; const uint8_t *data; } sfx_t;   /* ch: SFX_CH1 or SFX_CH4 */

/* steps (audio.h) — each step lasts `frames` frames (1/60 s) */
#define SFX_TONE(frames, sweep, duty, env, note) (frames), (sweep), (duty), (env), NOTE_##note
#define SFX_NOISE(frames, env, poly)             (frames), (env), (poly)
#define SFX_END 0
```
Register cheat sheet:
- `sweep` (NR10): `0x00` none. `0bTTTDSSS`: T = time 1–7 (slower as it grows), D = 0 up / 1 down, S = shift 1–7 (bigger = smaller change). Up: `0x15`, `0x26`; down: `0x1D`, `0x2E`.
- `duty` (NR11): `0x00` 12.5% · `0x40` 25% · `0x80` 50% · `0xC0` 75%.
- `env` (NR12/NR42): `0xVDP`… high nibble = start volume 0–F, bit 3 = 1 grow / 0 fade, low 3 bits = speed (1 fast … 7 slow, 0 = hold). `0xF1` loud short blip, `0xF3` loud medium, `0xA7` long fade, `0x81` quiet tick.
- `poly` (NR43): high nibble = pitch shift (0 = hiss, F = rumble), bit 3 = 1 metallic/tonal, low 3 bits = divider. `0x00` hiss, `0x40` crunch, `0x6D` thump, `0x74` explosion rumble.

```c
#include "engine/engine.h"
static const uint8_t d_jump[]  = { SFX_TONE(10, 0x15, 0x80, 0xF3, A4), SFX_END };
static const uint8_t d_coin[]  = { SFX_TONE(4, 0x00, 0x80, 0xF1, B5), SFX_TONE(12, 0x00, 0x80, 0xF3, E6), SFX_END };
static const uint8_t d_menu[]  = { SFX_TONE(3, 0x00, 0x40, 0xA1, C6), SFX_END };
static const uint8_t d_hurt[]  = { SFX_TONE(12, 0x2E, 0x40, 0xF2, E5), SFX_END };
static const uint8_t d_hit[]   = { SFX_NOISE(8, 0xF1, 0x40), SFX_END };
static const uint8_t d_boom[]  = { SFX_NOISE(4, 0xF1, 0x20), SFX_NOISE(40, 0xF7, 0x74), SFX_END };
const sfx_t sfx_jump = { SFX_CH1, 1, d_jump };
const sfx_t sfx_coin = { SFX_CH1, 2, d_coin };
const sfx_t sfx_menu = { SFX_CH1, 1, d_menu };
const sfx_t sfx_hurt = { SFX_CH1, 3, d_hurt };
const sfx_t sfx_hit  = { SFX_CH4, 2, d_hit };
const sfx_t sfx_boom = { SFX_CH4, 3, d_boom };
```
A new SFX replaces the one playing on the same channel only if its `prio` is ≥ the current one.

### 6.9 assets/assets.h

```c
#ifndef ASSETS_H
#define ASSETS_H
#include "engine/engine.h"
extern const sprite_def_t spr_player;
extern const tileset_def_t ts_forest;
extern const map_def_t map_level1, map_title;
extern const uint8_t font_main[], font_box_tiles[];
extern const song_t mus_theme;
extern const sfx_t sfx_jump, sfx_coin, sfx_menu, sfx_hurt, sfx_hit, sfx_boom;
#endif
```

---

## 7. Engine API

Build the engine exactly with these names and semantics. Reference code is given where correctness is subtle; write the rest in the same style.

### 7.1 core (core.h / core.c)

```c
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <rand.h>
#include <gbdk/emu_debug.h>

/* fixed point 12.4 in int16_t: 1 pixel = 16 units, range ±2047 px */
#define FIX(px)     ((int16_t)((px) << 4))
#define UNFIX(v)    ((int16_t)(v) >> 4)
#define ABS(a)      ((a) < 0 ? -(a) : (a))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define CLAMP(v,lo,hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

void engine_init(void);
int16_t approach(int16_t v, int16_t target, int16_t step);  /* move v toward target by step */
void rand_seed(void);     /* initrand(DIV_REG | (sys_time << 8)); call when the player presses START */
```
`engine_init()`:
```c
void engine_init(void) {
    if (_cpu != CGB_TYPE) { while (1) vsync(); }   /* Color only */
    cpu_fast();
    DISPLAY_OFF;                 /* the only allowed DISPLAY_OFF */
    SPRITES_8x8;
    fade_init();                 /* all palettes in RAM, fade level = fully black */
    scene_reset_screen();        /* clear BG map + attributes, hide sprites/window */
    text_init();                 /* font + box tiles into VRAM bank 1, default UI palette */
    audio_init();                /* sound on, add_VBL(audio_update) */
    SHOW_BKG; SHOW_SPRITES; HIDE_WIN;
    DISPLAY_ON;
}
```

### 7.2 input

```c
extern uint8_t keys, keys_prev;
void input_update(void);                       /* keys_prev = keys; keys = joypad(); */
#define KEY_HELD(k)     (keys & (k))
#define KEY_PRESSED(k)  ((keys & (k)) && !(keys_prev & (k)))
#define KEY_RELEASED(k) (!(keys & (k)) && (keys_prev & (k)))
/* k: J_UP J_DOWN J_LEFT J_RIGHT J_A J_B J_START J_SELECT (can be OR-ed) */
uint8_t input_wait_press(uint8_t mask);        /* blocking; returns the key pressed */
```

### 7.3 gfx (palettes + sprite loading)

```c
typedef struct { uint8_t base, w, h, tpf, frames, pal; } sprite_t;   /* RAM handle (tpf = tiles per frame) */

void gfx_set_bkg_palette(uint8_t slot, const palette_color_t *c4);  /* goes through fade (7.10) */
void gfx_set_obj_palette(uint8_t slot, const palette_color_t *c4);
uint8_t gfx_load_sprite(sprite_t *out, const sprite_def_t *def, uint8_t bank);
    /* copies tiles into bank-0 VRAM at next free index (0..127), loads palette into def->pal_slot,
       fills *out, returns 0 if VRAM is full. bank = 0 for non-banked data (section 10) */
void gfx_reset(void);                     /* sprite VRAM allocator back to 0 (called on scene change) */
```
Loading the same `sprite_def_t` twice wastes VRAM — load each once per scene and share the handle.

### 7.4 sprites (OAM allocator + drawing)

```c
void spr_begin(void);                          /* called by scene_update */
void spr_end(void);                            /* hides OAM entries not used this frame */
uint8_t spr_put(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop);   /* one 8x8, screen coords */
void spr_draw(const sprite_t *s, uint8_t frame, int16_t sx, int16_t sy, uint8_t flags);
void spr_hide_all(void);
#define SPR_FLIPX 0x20
#define SPR_FLIPY 0x40
#define SPR_BEHIND 0x80
```
Reference:
```c
static uint8_t spr_next, spr_last;
void spr_begin(void) { spr_next = 0; }
void spr_end(void) {
    uint8_t i;
    for (i = spr_next; i < spr_last; i++) hide_sprite(i);
    spr_last = spr_next;
}
uint8_t spr_put(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop) {
    if (spr_next >= 40 || sx <= -8 || sx >= 160 || sy <= -8 || sy >= 144) return 0;
    set_sprite_tile(spr_next, tile);
    set_sprite_prop(spr_next, prop);
    move_sprite(spr_next, (uint8_t)(sx + 8), (uint8_t)(sy + 16));
    spr_next++;
    return 1;
}
void spr_draw(const sprite_t *s, uint8_t frame, int16_t sx, int16_t sy, uint8_t flags) {
    uint8_t r, c, sr, sc;
    uint8_t first = s->base + frame * s->tpf;
    uint8_t prop = s->pal | flags;
    for (r = 0; r < s->h; r++) {
        sr = (flags & SPR_FLIPY) ? (uint8_t)(s->h - 1 - r) : r;
        for (c = 0; c < s->w; c++) {
            sc = (flags & SPR_FLIPX) ? (uint8_t)(s->w - 1 - c) : c;
            spr_put(first + sr * s->w + sc, sx + (c << 3), sy + (r << 3), prop);
        }
    }
}
```
Draw order = priority: sprites drawn **first appear on top**. Draw the player (and UI cursor) before enemies and particles.

### 7.5 anim

```c
typedef struct { const uint8_t *frames; uint8_t len; uint8_t speed; uint8_t loop; } anim_def_t; /* speed = frames per step */
typedef struct { const anim_def_t *def; uint8_t i, timer, done; } anim_t;
void anim_play(anim_t *a, const anim_def_t *def);   /* restarts only if def changed */
void anim_update(anim_t *a);                        /* once per frame */
#define anim_frame(a) ((a)->def->frames[(a)->i])
```
Defined in game code:
```c
static const uint8_t walk_frames[] = { 0, 1, 0, 2 };
static const anim_def_t anim_walk = { walk_frames, 4, 8, 1 };
```

### 7.6 map + camera

```c
void map_load(const map_def_t *m, uint8_t bank);   /* builds lookup tables, loads tileset (BG 128+) and BG palettes */
char    map_char(uint16_t tx, uint16_t ty);
uint8_t map_flags(uint16_t tx, uint16_t ty);       /* outside left/right/top = TF_SOLID, below = 0 */
uint8_t map_flags_px(int16_t px, int16_t py);      /* same, world pixel coords */
uint8_t map_find(char ch, uint8_t n, uint16_t *tx, uint16_t *ty);  /* n-th occurrence, returns 0 if none */
void    map_set_tile(uint16_t tx, uint16_t ty, char ch);  /* changes the VRAM tile only (visual: open door, collected coin);
                                                            keep game-side state for logic */
extern uint16_t map_w_px, map_h_px;

extern int16_t cam_x, cam_y;                       /* world pixel of screen top-left */
void cam_set(int16_t x, int16_t y);                /* clamp + redraw full screen (use at scene start) */
void cam_follow(int16_t wx, int16_t wy);           /* center on point with small dead zone, clamp, stream */
void cam_shake(uint8_t frames, uint8_t strength);
#define W2S_X(wx) ((wx) - cam_x)                   /* world -> screen */
#define W2S_Y(wy) ((wy) - cam_y)
```
Implementation notes:
- Lookup tables in RAM, built by `map_load`: `lut_tile[128]` (`128 + legend.tile`), `lut_attr[128]` (`pal | (flags & 0xE0)`), `lut_flags[128]`. Unknown chars → blank, flags 0.
- `map_char` = `rows[ty][tx] & 0x7F` (with bank switching when `bank != 0`).
- Maps smaller than the screen are drawn once and centered or top-left; clamp camera to `0..map_w_px-160` / `0..map_h_px-144` (0 if negative).
- Camera may move at most 8 px per frame per axis.

Streaming reference (the BG map is 32×32 and wraps; we redraw only the new column/row):
```c
static uint16_t drawn_tx, drawn_ty;   /* tile of the currently drawn top-left */

static void put_cell(uint16_t mx, uint16_t my) {
    uint8_t t = TILE_BLANK, a = ATTR_BLANK;
    if (mx < map_w && my < map_h) { char ch = map_char(mx, my); t = lut_tile[ch]; a = lut_attr[ch]; }
    VBK_REG = 1; set_bkg_tile_xy(mx & 31, my & 31, a);
    VBK_REG = 0; set_bkg_tile_xy(mx & 31, my & 31, t);
}
static void draw_col(uint16_t mx, uint16_t my) { uint8_t i; for (i = 0; i < 19; i++) put_cell(mx, my + i); }
static void draw_row(uint16_t mx, uint16_t my) { uint8_t i; for (i = 0; i < 21; i++) put_cell(mx + i, my); }

static void cam_apply(void) {          /* after cam_x/cam_y changed and were clamped */
    uint16_t tx = cam_x >> 3, ty = cam_y >> 3;
    while (drawn_tx < tx) { drawn_tx++; draw_col(drawn_tx + 20, drawn_ty); }
    while (drawn_tx > tx) { drawn_tx--; draw_col(drawn_tx,      drawn_ty); }
    while (drawn_ty < ty) { drawn_ty++; draw_row(drawn_tx, drawn_ty + 18); }
    while (drawn_ty > ty) { drawn_ty--; draw_row(drawn_tx, drawn_ty); }
    move_bkg((uint8_t)(cam_x + shake_dx), (uint8_t)(cam_y + shake_dy));
}
/* cam_set: set drawn_tx/ty = cam tile, draw rows ty..ty+18 fully (21 cells each), then move_bkg */
```

### 7.7 collide

```c
typedef struct { int16_t x, y, vx, vy; uint8_t w, h; } body_t;   /* x,y,vx,vy fixed 12.4; w,h hitbox px */
#define HIT_LEFT 1
#define HIT_RIGHT 2
#define HIT_UP 4
#define HIT_DOWN 8
uint8_t rect_overlap(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah,
                     int16_t bx, int16_t by, uint8_t bw, uint8_t bh);   /* pixels */
uint8_t body_overlap(const body_t *a, const body_t *b);
uint8_t body_move(body_t *b);           /* moves with tile collision, returns HIT_* bits */
uint8_t body_on_ground(const body_t *b);
uint8_t body_touch_flags(const body_t *b); /* OR of flags of all tiles the hitbox covers (hazard, trigger) */
```
Reference (speed must stay below 8 px/frame, i.e. |v| < FIX(8)):
```c
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
```
Platformer: `vy += GRAVITY` (e.g. 5), clamp `vy` to `FIX(4)`, jump `vy = -FIX(3)`, variable jump: if A released while `vy < 0`, `vy >>= 1`. Top-down: no gravity, set `vx/vy` from the d-pad.

### 7.8 text, dialogs, menus

```c
#define FONT_TILE(c) ((uint8_t)(((c) >= 32 && (c) < 128) ? 128 + (c) - 32 : 128))
#define TILE_BLANK 128
#define ATTR_BLANK 0x0F                    /* bank 1 + palette 7 */
void text_init(void);                      /* expand 1bpp font to 2bpp glyph by glyph (16-byte buffer),
                                              VBK_REG = 1, set_bkg_data(128 + i, 1, buf); box tiles at 224 */
void text_set_colors(const palette_color_t *c4);   /* BG palette 7 */
void text_print(uint8_t x, uint8_t y, const char *s);      /* BG layer, tile coords relative to camera
                                                              top-left: x + (cam_x>>3), wrapped &31 */
void text_print_win(uint8_t x, uint8_t y, const char *s);  /* window layer */
void text_print_num(uint8_t x, uint8_t y, uint16_t n, uint8_t digits);   /* zero padded */
void text_clear(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void box_draw_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h);           /* bordered box on window */

void dialog_show(const char *text);        /* BLOCKING */
uint8_t dialog_choice(const char *prompt, const char * const *options, uint8_t count);  /* BLOCKING, returns index */
uint8_t menu_run(uint8_t x, uint8_t y, const char * const *options, uint8_t count);    /* BLOCKING, on BG, arrow cursor */
```
Writing a character: write tile `FONT_TILE(c)` with `VBK_REG = 0` and attribute `ATTR_BLANK` with `VBK_REG = 1`, then leave `VBK_REG = 0`.

`dialog_show` behavior: window box 20×5 tiles at the bottom (`move_win(7, 104)`, `SHOW_WIN`), 3 lines × 18 chars; typewriter 1 char every 2 frames (every frame while A held); automatic word wrap; `\n` = new line; `\f` = new page; when a page is full or text ends, blink the NEXT arrow and wait for `A`; plays `sfx_menu` per page; `HIDE_WIN` at the end. Its loop calls `vsync(); input_update();` itself (audio keeps playing via interrupt). Sprites stay frozen as last drawn.

HUD: the window always covers everything from its top line down to the bottom of the screen, so it can't be a top bar. Put the HUD at the bottom (`move_win(7, 136)` = 1 row), or draw a small HUD with sprites. While a dialog is open, the dialog takes over the window.

### 7.9 scene manager

```c
typedef struct { void (*enter)(void); void (*update)(void); void (*leave)(void); } scene_t;  /* leave may be 0 */
#define TRANS_NONE 0
#define TRANS_FADE_BLACK 1
#define TRANS_FADE_WHITE 2
void scene_start(const scene_t *first);
void scene_goto(const scene_t *next, uint8_t transition);   /* request; performed at end of this frame */
void scene_update(void);
void scene_reset_screen(void);   /* hide sprites + window, fill BG map with TILE_BLANK/ATTR_BLANK
                                    (fill_bkg_rect on VRAM bank 0 and 1), move_bkg(0,0) */
```
Reference:
```c
static const scene_t *cur, *pending;
static uint8_t pending_trans;

static void run_frame(void) { spr_begin(); cur->update(); spr_end(); }

void scene_start(const scene_t *first) {
    cur = first; cur->enter(); run_frame(); fade_in(4, 0);
}
void scene_update(void) {
    run_frame();
    if (!pending) return;
    uint8_t white = (pending_trans == TRANS_FADE_WHITE);
    if (pending_trans != TRANS_NONE) fade_out(4, white);
    if (cur->leave) cur->leave();
    scene_reset_screen(); gfx_reset(); tween_clear(); particles_clear(); cam_reset();
    cur = pending; pending = 0;
    cur->enter();
    run_frame();
    if (pending_trans != TRANS_NONE) fade_in(4, white); else fade_set_level(0);
}
```

### 7.10 fade

The engine keeps a RAM copy of all 16 palettes (`pal_ram[64]`: 0–31 BG, 32–63 OBJ) and a fade level 0 (normal) … 8 (fully black/white). `gfx_set_*_palette` writes to RAM and then applies the current level, so palettes loaded while faded out stay invisible.

```c
void fade_init(void);                                  /* all black, level 8 */
void fade_out(uint8_t frames_per_step, uint8_t to_white);   /* BLOCKING, 8 steps */
void fade_in(uint8_t frames_per_step, uint8_t from_white);  /* BLOCKING */
void fade_set_level(uint8_t level);
void fade_apply(void);
```
Reference:
```c
static palette_color_t blend(palette_color_t c, uint8_t s, uint8_t white) {
    uint8_t r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    if (white) { r += ((31 - r) * s) >> 3; g += ((31 - g) * s) >> 3; b += ((31 - b) * s) >> 3; }
    else       { r -= (r * s) >> 3;        g -= (g * s) >> 3;        b -= (b * s) >> 3; }
    return RGB(r, g, b);
}
void fade_apply(void) {
    static palette_color_t tmp[64];
    uint8_t i;
    for (i = 0; i < 64; i++) tmp[i] = fade_level ? blend(pal_ram[i], fade_level, fade_white) : pal_ram[i];
    set_bkg_palette(0, 8, tmp);
    set_sprite_palette(0, 8, tmp + 32);
}
void fade_out(uint8_t fps, uint8_t to_white) {
    uint8_t s, f;
    fade_white = to_white;
    for (s = 1; s <= 8; s++) { fade_level = s; vsync(); fade_apply(); for (f = 1; f < fps; f++) vsync(); }
}
/* fade_in: same with s = 7 down to 0 */
```

### 7.11 tween

```c
#define EASE_LINEAR 0
#define EASE_IN 1
#define EASE_OUT 2
#define EASE_INOUT 3
uint8_t tween_start(int16_t *target, int16_t to, uint8_t frames, uint8_t ease);  /* from = *target now */
void tween_update(void);            /* once per frame */
uint8_t tween_busy(const int16_t *target);
void tween_clear(void);             /* pool of 8 */
```
Easing tables (17 entries, progress 0..16 → 0..256), interpolate between entries:
```c
static const uint16_t ease_tbl[4][17] = {
  {0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256},
  {0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225,256},
  {0,31,60,87,112,135,156,175,192,207,220,231,240,247,252,255,256},
  {0,2,8,18,32,50,72,98,128,158,184,206,224,238,248,254,256},
};
/* p = (t << 8) / dur (0..256); i = p >> 4; e = tbl[i] + (((tbl[i+1]-tbl[i]) * (p & 15)) >> 4) (i < 16);
   *target = from + (int16_t)(((int32_t)(to - from) * e) >> 8); on the last frame set *target = to exactly */
```
Use for menus sliding in, cameras, UI bounce, moving platforms on rails, title logos.

### 7.12 particles

```c
typedef struct {
    const sprite_t *spr;   /* 1x1 sprite; frames play across the lifetime */
    uint8_t speed;         /* max initial speed, 1/16 px per frame (24 = 1.5 px) */
    int8_t  up;            /* added to initial vy (negative = upward burst) */
    int8_t  gravity;       /* added to vy every frame */
    uint8_t life;          /* frames */
} particle_style_t;
void particles_emit(int16_t wx, int16_t wy, uint8_t count, const particle_style_t *st);  /* world px */
void particles_update(void);   /* move + draw (call inside update, after the important sprites) */
void particles_clear(void);    /* pool of 12; emitting when full recycles the oldest */
```
Random velocity: `(int8_t)(rand() % (2 * speed + 1)) - speed`. Keep bursts ≤ 8 particles because of the 10-per-line limit.

### 7.13 audio

```c
void audio_init(void);          /* NR52 = 0x80; NR50 = 0x77; NR51 = 0xFF; CRITICAL { add_VBL(audio_update); } */
void audio_update(void);        /* VBL interrupt: music tick + sfx step */
void music_play(const song_t *s);   /* wrap state changes in CRITICAL { } */
void music_stop(void);
void sfx_play(const sfx_t *s);
```
Frequency table (register value = 2048 − 131072 / Hz), index 0 = C2 … 71 = B7:
```c
static const uint16_t note_freq[72] = {
    44, 157, 263, 363, 457, 547, 631, 711, 786, 856, 923, 986,               /* 2 */
    1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517, /* 3 */
    1547, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783, /* 4 */
    1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915, /* 5 */
    1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964, 1969, 1974, 1978, 1982, /* 6 */
    1985, 1989, 1992, 1995, 1998, 2001, 2004, 2006, 2009, 2011, 2013, 2015, /* 7 */
};
```
Note enum in audio.h, in this order: `NOTE_C2, NOTE_Cs2, NOTE_D2, NOTE_Ds2, NOTE_E2, NOTE_F2, NOTE_Fs2, NOTE_G2, NOTE_Gs2, NOTE_A2, NOTE_As2, NOTE_B2, NOTE_C3, …, NOTE_B7` (72 values, 0–71).

Instrument tables:
```c
enum { INST_LEAD, INST_SQUARE, INST_THIN, INST_PLUCK, INST_SOFT, INST_ECHO };
static const uint8_t pulse_inst[][2] = {     /* duty (NRx1), envelope (NRx2) */
    {0x80, 0xC4}, {0x80, 0xA0}, {0x00, 0xA3}, {0x40, 0xF1}, {0x40, 0x75}, {0x80, 0x42} };

enum { WAVE_TRI, WAVE_SAW, WAVE_SQUARE, WAVE_SOFTTRI };
static const uint8_t wave_inst[][2] = {      /* wave index, NR32 volume (0x20 full, 0x40 half) */
    {0, 0x20}, {1, 0x40}, {2, 0x40}, {0, 0x40} };
static const uint8_t waves[3][16] = {
    {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10},  /* triangle */
    {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF},  /* saw */
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* square */
};

enum { DRUM_KICK, DRUM_SNARE, DRUM_HAT, DRUM_OHAT, DRUM_CRASH, DRUM_TOM };
static const uint8_t drums[][2] = {          /* envelope (NR42), poly (NR43) */
    {0xF1, 0x6D}, {0xD2, 0x42}, {0x81, 0x00}, {0x83, 0x00}, {0xF5, 0x21}, {0xC2, 0x5D} };
```
Driver rules:
- Per channel state: `pos`, `loop`, `wait`, `inst`. Every `fpt` frames = one tick. When `wait` reaches 0, read commands until a note/drum/rest, then `wait = len`.
- Note trigger CH1: `NR10 = 0; NR11 = duty; NR12 = env; NR13 = f & 0xFF; NR14 = 0x80 | (f >> 8)`. CH2: `NR21..NR24` the same.
- CH3: use `note_freq[MIN(note + 12, 71)]` (CH3 sounds an octave lower). If the wave changed: `NR30 = 0`, copy 16 bytes to `0xFF30`–`0xFF3F`. Then `NR30 = 0x80; NR32 = vol; NR33 = f & 0xFF; NR34 = 0x80 | (f >> 8)`.
- CH4: `NR42 = env; NR43 = poly; NR44 = 0x80`.
- Rest: CH1 `NR12 = 0`, CH2 `NR22 = 0`, CH3 `NR30 = 0`, CH4 `NR42 = 0`.
- While an SFX owns a channel, music still advances on it but skips all register writes.
- SFX step: CH1 `NR10 = sweep; NR11 = duty; NR12 = env; NR13/NR14` from the note table with trigger. CH4: `NR42 = env; NR43 = poly; NR44 = 0x80`. On `SFX_END`: silence the channel and release it.
- **Music and SFX data must not be banked** (the interrupt reads them at any time).

---

## 8. Art direction for AI-drawn pixels

- Think in 8×8 tiles. Before writing `PX` rows, sketch the full sprite as a grid in a comment, then split into tiles.
- Silhouette first: a 1-pixel dark outline (index 1) makes sprites readable on any background.
- Sprites: index 1 outline, 2 main color, 3 highlight/skin. Index 0 is transparent — never use it for a visible pixel.
- BG tiles: index 0 is the most common/background color of that palette, so empty-looking tiles stay cheap to read.
- Keep tiles that repeat (ground, walls) seamless: check that the right column matches the left column and bottom matches top when tiled.
- Contrast: sprites should use more saturated/brighter colors than the BG so they pop.
- Walk cycles: 2–4 frames; for left/right, draw one direction and use `SPR_FLIPX`.
- Reuse: flipped tiles (`TF_FLIPX`/`TF_FLIPY`) for symmetric corners saves tileset space.
- Colors: pick 4-color ramps (dark → light) with slight hue shift. Avoid pure #000/#FFF except for text UI.

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

## 10. ROM banking (only when the game grows past 32 KB)

The linker prints an error about ROM/area overflow when non-banked code+data exceed 32 KB. Then:
1. Keep **all code, music, SFX and font non-banked**.
2. Move large **maps, tilesets and sprite data** to banked files: add `#pragma bank 255` as the first line and `BANKREF(name)` for the main struct; in `assets.h` use `BANKREF_EXTERN(name)`.
3. Everything a load call touches must be **in the same banked file** (a map file includes its own tileset rows/tiles/palettes; a sprite file its tiles and palette).
4. Load with the bank: `map_load(&map_level5, BANK(map_level5));` `gfx_load_sprite(&h, &spr_boss, BANK(spr_boss));`
5. Engine loaders switch and restore: `uint8_t save = CURRENT_BANK; if (bank) SWITCH_ROM(bank); ... if (bank) SWITCH_ROM(save);`. The map module stores the bank and switches in `map_char` and while streaming.

## 11. Composing music and SFX

- Channel roles: **CH2 melody**, **CH1 harmony/counter-melody** (SFX interrupt it), **CH3 bass**, **CH4 drums** (noise SFX interrupt it).
- Write in a key; keep melody in octaves 4–6, bass in octaves 2–3.
- Structure: 4- or 8-bar phrases, intro before `MUS_LOOP_POINT`, loop length 8–32 bars.
- Comment each bar with its tick total (`/* bar 3: 16 */`). Verify every channel's loop sums to the same length (or a divisor).
- Mood guide: title = catchy, 120–150 BPM; exploration = calm, 90–112 BPM, `INST_SOFT`; battle = fast 150–180 BPM, `INST_THIN` lead, busy drums; game over = slow, minor key, no loop (`MUS_END`).
- SFX: short (3–20 frames), higher priority for important feedback (hurt 3, coin 2, jump 1). Rising sweeps = positive (jump, power-up); falling = negative (hurt, lose).

## 12. Checklists

### 12.1 Scaffold order (new project)
1. `build.bat`, `build.sh`, `run.bat`, `.vscode/*`, `.gitignore`, `GAME.md`.
2. Engine in this order: `tiles.h`, `core`, `input`, `fade`, `gfx`, `sprites`, `text` (+ `font_main.c` with all 96 glyphs and box tiles), `scene`, `audio`, `anim`, `map`, `collide`, `tween`, `particles`, `engine.h`.
3. `assets.h`, a title map, `scenes/title.c`, `main.c`. Build and fix.
4. Then the actual game.

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

## 13. Debugging and troubleshooting

- `EMU_printf("x=%d y=%d", x, y);` (from `<gbdk/emu_debug.h>`) prints to Emulicious's debug console; remove it for release.
- Emulicious: *Tools → Tile Viewer / Palette Viewer / Tilemap Viewer / Sprite Viewer* shows exactly what is in VRAM.

| Symptom | Likely cause |
|---|---|
| Garbage tiles | `PX` row count wrong, wrong tile index, loaded to the wrong VRAM bank (`VBK_REG` left at 1) |
| Wrong colors on BG | Attribute palette not set (bank 1 write missing) or palette slot not loaded |
| Sprites flicker/disappear | More than 10 sprites on a line or more than 40 total |
| Text shows as blocks | Font not loaded in bank 1, or attribute bit 3 missing |
| Screen white flash | `DISPLAY_OFF` used outside `engine_init` |
| Music drifts out of sync | Channel loop lengths differ |
| Slowdown | Heavy math per frame, too many entities, `%`/`/` in loops, full-screen redraws every frame |
| Crash after adding data | ROM overflow into bank switching problems → section 10 |
| `?ASlink-Warning-Undefined Global` | Missing `.c` file, typo, or declared in `assets.h` but never defined |
| Build error "multiple definition" | Two `.c` files with the same base name, or data defined in a header |
