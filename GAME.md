# SOKOBAN — Warehouse Keeper

A small Sokoban puzzle for the Game Boy Color with a single level.

## Genre and goal
Grid puzzle. Push every crate onto a yellow goal mark. Crates can only be pushed (never pulled), one at a time.

## Controls
| Button | Action |
|---|---|
| D-pad | Walk / push (hold to keep walking) |
| B | Undo last move (up to 64 moves back) |
| SELECT | Restart the level |
| START | Pause menu: Continue / Restart / Quit to title |
| START or A | Confirm on title and solved screens |

## Scenes
1. **title** (`src/scenes/title.c`): `map_title`, "S O K O B A N / WAREHOUSE KEEPER", blinking PRESS START, keeper sprite pushing a crate. Music `mus_title`.
2. **level** (`src/scenes/level.c`): `map_level1`, HUD on the window (bottom row): `MOVES 000   PUSH 000`. Help dialog once per power-on. When solved: `mus_solved` fanfare, dialog, fade to white.
3. **solved** (`src/scenes/solved.c`): `map_title` background, move/push counts, hopping keeper. Music `mus_title`. START returns to title.

## Game logic
- `src/game/board.c`: board of 10×9 cells, each cell = 16×16 px = 2×2 map tiles. Cells read from the map's top-left char of each 2×2 block: `#`/space = wall, `a` = goal, `B` = crate, `W` = crate on goal, `P` = player.
- Crates live on the **background** (redrawn with `map_set_tile`); while a crate slides it is removed from the BG and drawn as the `spr_crate` sprite for 8 frames, then written back. Crates on goals switch to the green palette.
- Undo history: 64-entry ring buffer of `dir | pushed`.
- `src/game/state.c`: `game_moves`, `game_pushes` of the last solve (shown on the solved screen).

## The level (cell view)
```
          
 ######## 
 #  #   # 
 # $  $ # 
 #  ##  # 
 #.$@ $.# 
 #  ..  # 
 ######## 
          
```
4 crates, 4 goals. Verified solvable, optimal solution 39 moves.

## Assets
| File | Content |
|---|---|
| `assets/palettes/pal_warehouse.c` | BG palettes: wall, floor, goal, crate, crate-on-goal, void |
| `assets/tilesets/ts_sokoban.c` | 8 tiles: 0 void, 1 brick wall, 2 floor, 3 goal quarter (flipped ×4), 4–7 crate quarters |
| `assets/maps/map_level1.c` | 20×18 level map (legend: ` # - P a b c d B C D E W X Y Z`) |
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
