# Source art pipeline (PNG → C assets)

Read when: `source_art/` contains PNGs to convert (new or replaced). The helper script is in [png2gb.md](png2gb.md).

## 14. Source art pipeline (PNG → C assets)

The human can draw in any pixel editor and paste PNGs into `source_art/`. You turn them into the normal C assets of section 6 ([graphics.md](graphics.md)); from then on the C file is the asset and the PNG is only its source. Formats the human must follow, and example notes files, are in `source_art/spritesheets/file.txt` and `source_art/mapsheets/file.txt`.

| Put in | Becomes |
|---|---|
| `source_art/spritesheets/<name>.png` (+ `<name>.txt`) | `assets/sprites/spr_<name>.c` (tiles, palette, `sprite_def_t`) + `anim_def_t` frame lists in the game code that uses it |
| `source_art/mapsheets/<name>.png` (+ `<name>.txt`) | `assets/maps/map_<name>.c` (rows + legend) + `assets/tilesets/ts_<tileset>.c` (tiles + BG palettes, shared by maps with the same `tileset:`) |

### 14.1 Reading a PNG

1. **Look** at the image with the Read tool first: understand what it shows, the frame grid, which parts are background.
2. **Measure** exact pixels with the helper script in [png2gb.md](png2gb.md). Copy it into the scratchpad/temp folder (never into the project) and run it with Node. It needs no npm packages.
   - `node png2gb.js <png> colors` — size, tile count, every color with its `RGB8()` and pixel count.
   - `node png2gb.js <png> grid [--scale N] [--rect x,y,w,h]` — one symbol per pixel (`.` = transparent, `0-9a-zA-Z` = colors, most used first). `--rect` is in scaled pixels: use it to dump one frame at a time.
   - `node png2gb.js <png> tiles [--scale N]` — unique 8×8 tiles (a tile and its X/Y/XY flips count once), each tile's color set, the distinct color sets, and the tile map (`id` + `X`/`Y`/`XY` flip).
3. Symbols are **not** palette indices. You decide the mapping (e.g. `.`→0, darkest→1, main→2, lightest→3) and write it in a comment of the generated file.
4. Transparency = alpha < 128 or magenta `#FF00FF`. Supported: non-interlaced, 1–8 bit, grayscale/RGB/indexed/alpha. The script says what to ask the human to re-save otherwise.

### 14.2 Sprite sheets → `spr_<name>.c`

1. Frame size, scale, rows/animations and `pal_slot` come from `<name>.txt`; if there is no notes file, infer them from the image and write your guesses into `GAME.md`.
2. Dump each frame with `grid --rect`. Frame width/height must be multiples of 8; if the art is smaller, pad with transparent pixels (keep the feet on the bottom row).
3. ≤ 3 visible colors per sprite (one OBJ palette). If the image has more, merge the closest colors (anti-aliasing, near-duplicates) and tell the human which ones you merged. If the notes ask for more colors, split into an overlay sprite with a second palette.
4. Palette: index 0 transparent (any color, e.g. `RGB8(0,0,0)`), 1 darkest (outline), 2 main, 3 lightest, using the PNG's exact colors in `RGB8()`.
5. Frames go into `spr_<name>_tiles[]` in sheet order (row by row, left to right), each frame's tiles row-major, with a comment per frame (`/* frame 5: walk 2 (row 1, col 1) */`). Keep the pixel sketch of each frame from the `grid` dump in comments when it helps.
6. Frames facing left in the sheet are not needed: skip them if they are exact mirrors and use `SPR_FLIPX`.
7. Write the `anim_def_t` lists from the notes where the game code uses the sprite (`speed` = frames per step).
8. Check: tile count = `w * h * frames`, ≤ 128 sprite tiles per scene in total.

### 14.3 Map sheets → `ts_<tileset>.c` + `map_<name>.c`

1. Run `tiles`. Limits: ≤ 128 unique tiles per tileset (all maps sharing it, together), each tile ≤ 4 colors, ≤ 7 BG palettes per scene.
2. **Palettes**: group the listed color sets into ≤ 7 palettes of 4 colors (a set that is a subset of another shares its palette). Order each palette light → dark (index 0 = the most common/background color). Too many sets → merge near-identical colors first, then ask the human which colors may change; never silently repaint.
3. **Tileset**: one `PX` tile per unique tile, in the helper's order, commented with its index and a name (`/* 12: brick top-left */`). Tile pixels use the palette index of their color in the palette that tile belongs to.
4. **Legend**: one character per (tile, palette, flags) combination. Pick readable characters (`#` wall, `.` floor/sky, `=` platform, `^` spikes, `~` water, letters for the rest); only printable ASCII 33–126 plus space, at most ~90 combinations. Flipped tiles get their own character with `TF_FLIPX`/`TF_FLIPY`.
5. **Game meaning** comes from the notes (`solid`, `oneway`, `hazard`, `over`, `trigger`) → `TF_*` flags on the legend entries. Spawns (`spawn:` tile coordinates) become `TF_SPAWN` characters placed at those cells, drawn as the tile underneath (add a legend entry per spawn char, e.g. `'P'` drawn as the floor tile).
6. **Rows**: write the tile map as `w`-character strings, with the column ruler comment of 6.5. Verify with Node that every row has exactly `w` characters and every character has a legend entry.
7. `type: title` (or any 160×144 image) → a 20×18 map for a title/menu screen. Remember rows under text are overwritten by `text_print` at run time, and BG palette 7 is the UI palette.
8. `type: tiles` (a tile sheet, no map) → only the tileset; draw the maps yourself in ASCII from the notes.

### 14.4 After converting

- First line comment of every generated asset: `/* Generated from source_art/spritesheets/player.png — edit the PNG and reconvert, or edit here and keep both in sync. */`
- Update `assets.h`, the "Source art" table in `GAME.md`, build, and tell the human what you merged, padded or guessed, and what to check in Emulicious (Tile Viewer / Palette Viewer).
- If the human later edits the C file by asking you (not the PNG), say that the PNG is now out of date.
