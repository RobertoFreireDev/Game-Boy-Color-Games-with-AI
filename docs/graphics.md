# Graphics asset formats and art direction

Read when: writing or editing palettes, sprites, tilesets, maps, the font, or `assets.h`, or drawing any pixel art.

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
Animations are **not** stored here; they are tiny frame lists in game code (section 7.5, [engine-api.md](engine-api.md)).

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
/* UI blip used by dialog_show / dialog_choice / menu_run */
static const uint8_t d_menu[] = { SFX_TONE(3, 0x00, 0x40, 0xA1, C6), SFX_END };
const sfx_t sfx_menu = { SFX_CH1, 1, d_menu };
```
This file already exists and is shared by every game: don't rewrite or delete it. Restyle the font or the UI blip in place if a game needs it.

### 6.9 assets/assets.h

```c
#ifndef ASSETS_H
#define ASSETS_H
#include "engine/engine.h"
extern const sprite_def_t spr_player;
extern const tileset_def_t ts_forest;
extern const map_def_t map_level1, map_title;
extern const uint8_t font_main[], font_box_tiles[];   /* always present (font_main.c) */
extern const song_t mus_theme;
extern const sfx_t sfx_menu;                          /* always present (font_main.c) */
extern const sfx_t sfx_jump, sfx_coin, sfx_hurt, sfx_hit, sfx_boom;
#endif
```
In the template, `assets.h` holds only the two "always present" lines under empty section comments; add a game's externs under them.

## 8. Art direction for AI-drawn pixels

These apply to art you draw yourself and to cleaning up converted art (section 14, [source-art.md](source-art.md)): never change the human's drawing beyond what the hardware forces, but do apply these rules when you fill in missing frames or tiles.

- Think in 8×8 tiles. Before writing `PX` rows, sketch the full sprite as a grid in a comment, then split into tiles.
- Silhouette first: a 1-pixel dark outline (index 1) makes sprites readable on any background.
- Sprites: index 1 outline, 2 main color, 3 highlight/skin. Index 0 is transparent — never use it for a visible pixel.
- BG tiles: index 0 is the most common/background color of that palette, so empty-looking tiles stay cheap to read.
- Keep tiles that repeat (ground, walls) seamless: check that the right column matches the left column and bottom matches top when tiled.
- Contrast: sprites should use more saturated/brighter colors than the BG so they pop.
- Walk cycles: 2–4 frames; for left/right, draw one direction and use `SPR_FLIPX`.
- Reuse: flipped tiles (`TF_FLIPX`/`TF_FLIPY`) for symmetric corners saves tileset space.
- Colors: pick 4-color ramps (dark → light) with slight hue shift. Avoid pure #000/#FFF except for text UI.
