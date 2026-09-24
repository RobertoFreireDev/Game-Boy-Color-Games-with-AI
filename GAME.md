# SOKOBAN — Warehouse Keeper

A Sokoban puzzle for the Game Boy Color with 16 levels, level select and battery-saved progress.

## Genre and goal
Grid puzzle. Push every crate onto a yellow goal mark. Crates can only be pushed (never pulled), one at a time.

## Controls
| Button | Action |
|---|---|
| D-pad | Walk / push (hold to keep walking) |
| B | Undo last move (up to 64 moves back) |
| SELECT | Restart the level |
| START | Pause menu: Continue / Restart / Quit to title |
| LEFT / RIGHT | Title: choose any unlocked level |
| START or A | Title: play the selected level · Solved screen: next level |
| B | Solved screen: back to title |

## Scenes
1. **title** (`src/scenes/title.c`): `map_title`, "S O K O B A N / WAREHOUSE KEEPER", blinking PRESS START, level select `< LEVEL 01 >` (row 13, up to the highest unlocked level; starts on it), keeper sprite pushing a crate. Loads the save once per power-on. Music `mus_title`.
2. **level** (`src/scenes/level.c`): plays `game_level`. HUD on the window (bottom row): `LEVEL 01 M 000 P 000`. Help dialog once per power-on. When solved: records the result (best moves, unlocks next level, writes save), `mus_solved` fanfare, dialog, fade to white.
3. **solved** (`src/scenes/solved.c`): `map_title` background, `LEVEL nn SOLVED!`, moves/pushes, best (or NEW BEST!), `NEXT: LEVEL nn` or `ALL LEVELS DONE!`, hopping keeper. START → next level (title after level 16), B → title. Music `mus_title`.

## Game logic
- `src/game/board.c`: board of 10×9 cells, each cell = 16×16 px = 2×2 map tiles. `board_load(level)` reads the level's cell strings from `map_levels` (`#` wall, space void, `-` floor, `.` goal, `$` crate, `*` crate on goal, `@` player, `+` player on goal), builds a 20×18 tile map in RAM (`map_buf`, 360 bytes) and `map_load`s it. Cell row 8 is always void (half covered by the HUD).
- Crates live on the **background** (redrawn with `map_set_tile`); while a crate slides it is removed from the BG and drawn as the `spr_crate` sprite for 8 frames, then written back. Crates on goals switch to the green palette.
- Undo history: 64-entry ring buffer of `dir | pushed`.
- `src/game/state.c`: `game_level`, `game_unlocked`, `game_best[16]` (fewest moves, 0 = unsolved), last solve's `game_moves`/`game_pushes`. Saved to battery RAM at `0xA000` (magic `0x5B0C` + checksum); a bad/empty save starts fresh with level 1 unlocked.

## Levels
All 16 live in `assets/maps/map_levels.c` (8 strings × 10 chars each, centered). Every one was checked solvable with a BFS solver; difficulty rises from 1 crate / 3 moves to 5 crates / 26 pushes.

| # | Name | Crates | Pushes / moves (push-optimal) |
|---|---|---|---|
| 1 | First Push | 1 | 2 / 3 |
| 2 | Two Step | 2 | 2 / 4 |
| 3 | Side Door | 2 | 3 / 15 |
| 4 | Pillar | 2 | 4 / 27 |
| 5 | Corridor | 2 | 10 / 46 |
| 6 | Crossroads | 4 | 8 / 43 |
| 7 | Loop | 2 | 11 / 59 |
| 8 | Three Lanes | 3 | 14 / 61 |
| 9 | Warehouse (the original level) | 4 | 10 / 60 |
| 10 | Stairs | 3 | 13 / 42 |
| 11 | Funnel | 3 | 16 / 38 |
| 12 | Hook | 3 | 13 / 66 |
| 13 | Five Crates | 5 | 13 / 66 |
| 14 | Grid | 4 | 16 / 65 |
| 15 | Octagon | 4 | 17 / 95 |
| 16 | Grand Hall | 5 | 26 / 56 |

Max level size: 10×8 cells including walls.

## Assets
| File | Content |
|---|---|
| `assets/palettes/pal_warehouse.c` | BG palettes: wall, floor, goal, crate, crate-on-goal, void |
| `assets/tilesets/ts_sokoban.c` | 8 tiles: 0 void, 1 brick wall, 2 floor, 3 goal quarter (flipped ×4), 4–7 crate quarters |
| `assets/maps/map_levels.c` | 16 levels in cell view + the play-map legend (` # - a b c d B C D E W X Y Z`) |
| `assets/maps/map_title.c` | 20×18 title / solved background |
| `assets/sprites/spr_player.c` | Keeper 16×16, 6 frames (down, down-step, up, up-step, right, right-step), OBJ pal 0 |
| `assets/sprites/spr_crate.c` | Sliding crate 16×16, 1 frame, OBJ pal 1 |
| `assets/fonts/font_main.c` | 96-glyph 5×7 font + dialog box tiles |
| `assets/music/mus_title.c` | C major, 150 BPM, 8-bar loop |
| `assets/music/mus_puzzle.c` | A minor, 100 BPM, calm 8-bar loop |
| `assets/music/mus_solved.c` | Fanfare, plays once |
| `assets/sfx/sfx_all.c` | menu, start, step, push, bump, goal, undo |

## Palette plan
- BG 0 wall · 1 floor · 2 goal · 3 crate · 4 crate on goal (green) · 5 void · 7 UI text (background matches the void color).
- OBJ 0 player (red cap/overalls) · OBJ 1 sliding crate.

## VRAM / sprite budget
- Sprite tiles: player 24 + crate 4 = 28 / 128.
- BG tiles: 8 / 128.
- Sprites on screen: player 4 + sliding crate 4 = 8 / 40; max 4 per scanline.
