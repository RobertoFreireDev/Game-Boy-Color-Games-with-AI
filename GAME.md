# SOKOBAN — Warehouse Keeper

A Sokoban puzzle for the Game Boy Color with 16 levels, level select and battery-saved progress,
plus a side-scrolling **Bonus Run** whose job is to exercise the engine features Sokoban can't (see *Engine coverage*).

## Genre and goal
Grid puzzle. Push every crate onto a yellow goal mark. Crates can only be pushed (never pulled), one at a time.

## Controls
| Button | Action |
|---|---|
| D-pad | Walk / push (hold to keep walking) |
| B | Undo last move (up to 64 moves back) |
| SELECT | Restart the level (instant, no fade) |
| START | Pause menu: Continue / Restart / Quit to title |
| LEFT / RIGHT | Title: choose any unlocked level |
| START or A | Title: open the menu (PLAY LEVEL nn / BONUS RUN / HOW TO PLAY) · Solved screen: next level |
| B | Solved screen: back to title |
| LEFT / RIGHT | Bonus: run (accelerates / decelerates) |
| A | Bonus: jump (release early for a short hop) |
| UP | Bonus: read the sign / try the locked door |
| START | Bonus: pause (Continue / Quit to title) |

## Scenes
1. **title** (`src/scenes/title.c`): `map_title`, "S O K O B A N / WAREHOUSE KEEPER", blinking PRESS START (row 12), level select `< LEVEL 01 >` (row 14, up to the highest unlocked level; starts on it). The keeper walks in (tween) and pushes a crate; six stars twinkle over the walls (`spr_put`). START/A opens a `menu_run` menu: PLAY LEVEL nn, BONUS RUN, HOW TO PLAY (full-screen window box that slides up/down with tweens and closes on any button via `input_wait_press`). Loads the save once per power-on. Music `mus_title`.
2. **level** (`src/scenes/level.c`): plays `game_level`. HUD on the window (bottom row): `LEVEL 01 M 000 P 000`. Help dialog once per power-on. Goal marks pulse (BG palette 2 swapped every 16 frames). Bumping a wall shakes the camera. A crate landing on a goal bursts sparkles. When solved: records the result (best moves, unlocks next level, writes save), music stops, quick white flash, `mus_solved` fanfare, sparkle burst, the keeper spins twice (one-shot anim) then flashes gold (OBJ palette 0), dialog, fade to white.
3. **solved** (`src/scenes/solved.c`): `map_title` background, UI text in gold (`text_set_colors`, restored in the scene's `leave`). `LEVEL nn SOLVED!`, moves/pushes counting up (linear tweens), best (or NEW BEST!), `NEXT: LEVEL nn` or `ALL LEVELS DONE!`, keeper hopping (EASE_OUT up, EASE_IN down) with confetti every 4th landing. START → next level (title after level 16), B → title. Music `mus_title`.
4. **bonus** (`src/scenes/bonus.c`): side-scrolling platformer on `map_bonus` (80×24 tiles, camera streams both axes). Collect all 10 coins to open the exit door, then walk into it. Rats patrol (turning at walls, ledges and spikes); stomp them from above, touching them otherwise hurts. Spikes and the pit cost a heart (3 hearts; the pit respawns you at the last safe ground). A sign explains the rules. HUD on the window: `COINS 00/10   HP ***`. Clear → `mus_solved`, dialog, fade to white → title. Out of hearts → the keeper falls upside down, `mus_gameover`, dialog → title. Music `mus_bonus`.

## Bonus run
- Physics: `body_t` hitbox 10×14 inside the 16×16 keeper, gravity 4, max fall `FIX(4)`, jump `-FIX(4)` (~34 px = 4 tiles); releasing A while rising halves `vy`. Running is 1.5 px/frame via `approach` (accel 2, decel 3). Stomp bounce `-FIX(3)`. 60 invulnerable (blinking) frames after a hit.
- Map markers (`TF_SPAWN`, found with `map_find`): `P` start, `o` coin (max 12), `R` rat (max 4). `H h J j` exit door and `S` sign are `TF_TRIGGER`; which one is touched is decided with `map_char` at the keeper's centre.
- Tile flags: `#` bricks and `BCDE` crates `TF_SOLID`, `=` shelves `TF_ONEWAY`, `^` spikes `TF_HAZARD`, `F f` fence `TF_OVER` (bars in front of sprites). The pit has no floor, so you fall below the map.
- The opened door is drawn with `map_set_tile` (chars `K k L l`) every frame, because streaming redraws the closed door from the map rows. The keeper walking into the door is drawn with `SPR_BEHIND`, so the door frame covers him.
- Dying rats and the dying keeper are drawn with `SPR_FLIPY`.

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
| `assets/maps/map_title.c` | 20×18 title / solved background (text bands: rows 2-5 and 12-15) |
| `assets/maps/map_bonus.c` | 80×24 bonus run map + legend |
| `assets/tilesets/ts_bonus.c` | 16 tiles: back wall, brick, shelf, spikes, crate ×4, fence ×2, door closed ×2, door open ×2, sign, beam |
| `assets/palettes/pal_bonus.c` | Bonus BG palettes: back wall, wood, spikes, door, fence (bricks reuse `pal_wh_wall`) |
| `assets/palettes/pal_effects.c` | Swapped in at run time: goal glow, gold keeper, gold UI text |
| `assets/sprites/spr_player.c` | Keeper 16×16, 6 frames (down, down-step, up, up-step, right, right-step), OBJ pal 0 |
| `assets/sprites/spr_crate.c` | Sliding crate 16×16, 1 frame, OBJ pal 1 |
| `assets/sprites/spr_spark.c` | Sparkle 8×8, 3 frames (star, small star, dot), OBJ pal 6: particles and title stars |
| `assets/sprites/spr_coin.c` | Spinning coin 8×8, 4 frames, OBJ pal 2 |
| `assets/sprites/spr_rat.c` | Rat 16×8, 2 frames, faces left, OBJ pal 3 |
| `assets/fonts/font_main.c` | 96-glyph 5×7 font + dialog box tiles |
| `assets/music/mus_title.c` | C major, 150 BPM, 8-bar loop |
| `assets/music/mus_puzzle.c` | A minor, 100 BPM, calm 8-bar loop |
| `assets/music/mus_solved.c` | Fanfare, plays once |
| `assets/music/mus_bonus.c` | E minor, 150 BPM, 4-bar loop: INST_THIN lead, INST_ECHO echo, WAVE_SAW → WAVE_SQUARE bass, OHAT/TOM drums |
| `assets/music/mus_gameover.c` | A minor, 90 BPM, plays once: INST_PLUCK melody, WAVE_SOFTTRI bass |
| `assets/sfx/sfx_all.c` | menu, start, step, push, bump, goal, undo, jump, coin, hurt, stomp, door |

## Palette plan
- BG 0 wall · 1 floor · 2 goal · 3 crate · 4 crate on goal (green) · 5 void · 7 UI text (background matches the void color).
- BG 2 alternates `pal_wh_goal` / `pal_wh_goal_glow` in the level. BG 7 is gold (`pal_ui_gold`) on the solved screen.
- Bonus BG: 0 brick · 1 back wall · 2 wood (shelf, crate, sign) · 3 spikes · 4 door · 5 fence · 7 UI.
- OBJ 0 player (red cap/overalls; gold while celebrating) · OBJ 1 sliding crate · OBJ 2 coin · OBJ 3 rat · OBJ 6 sparkle.

## VRAM / sprite budget
- Level: sprite tiles player 24 + crate 4 + spark 3 = 31 / 128 · BG tiles 8 / 128 · sprites player 4 + crate 4 + particles ≤12 = 20 / 40.
- Bonus: sprite tiles player 24 + coin 4 + rat 4 + spark 3 = 35 / 128 · BG tiles 16 / 128 · sprites player 4 + rats 3×2 + coins ≤10 + particles ≤12 = 32 / 40 worst case (off-screen objects are not drawn).
- Title: player 24 + spark 3 tiles · 4 + 6 stars = 10 sprites.

## Engine coverage (what to look at in the emulator)
| Engine feature | Where it is exercised |
|---|---|
| `tween_*` (all 4 easings, `tween_busy`) | Title: keeper walks in (OUT), help window (INOUT up, IN down). Solved: stats count up (LINEAR), hop (OUT/IN) |
| `particles_*` | Level: crate on goal, solve burst. Bonus: coin sparkle, stomp dust. Solved: confetti |
| `anim_*` looping + one-shot (`done`) | Every scene loops; level solve spin is one-shot, the gold flash starts when `done` |
| `body_move`, `body_on_ground`, `body_overlap`, `body_touch_flags`, `rect_overlap`, `approach` | Bonus: keeper and rats, coins, hazards/triggers |
| `cam_follow` + streaming on both axes, `cam_shake` | Bonus (80×24 map). Shake: level wall bump, bonus hurt and door opening |
| `map_find`, `map_char`, `map_flags`, `map_flags_px`, `map_set_tile` | Bonus spawns, sign/door detection, rat ledge/spike checks, open door. Level crates |
| `TF_SOLID/ONEWAY/HAZARD/TRIGGER/SPAWN/OVER/FLIPX/FLIPY` | Bonus map (FLIPX on the door); goal quarters (FLIPX/FLIPY) |
| `spr_put`, `SPR_FLIPX/FLIPY/BEHIND` | Title stars; bonus dying keeper/rats (FLIPY); keeper entering the door (BEHIND) |
| `gfx_set_bkg_palette`, `gfx_set_obj_palette`, `text_set_colors` | Goal pulse, gold keeper, gold solved text |
| `fade_out`/`fade_in` called directly, `TRANS_NONE` (`fade_set_level`), `TRANS_FADE_WHITE` | Solve flash, SELECT restart, level → solved, bonus clear |
| scene `leave` | Solved screen restores the UI colors |
| `menu_run`, `box_draw_win`, `text_clear`, `input_wait_press`, `dialog_show`, `dialog_choice` | Title menu and help; blinking texts; dialogs and pause menus |
| `KEY_RELEASED` | Bonus variable jump height |
| `music_stop`, every instrument/wave/drum | Level solve; `mus_bonus` + `mus_gameover` cover INST_THIN/PLUCK/ECHO, WAVE_SAW/SQUARE/SOFTTRI, OHAT/TOM |
| **Not exercised:** ROM banking (`bank` argument of `map_load` / `gfx_load_sprite`) | The ROM is still 32 KB. Banking needs all non-banked code+data in the 16 KB bank 0, but it is already ~28 KB (see CLAUDE.md §10) |
