# Debugging and troubleshooting

Read when: a build fails, or something looks or sounds wrong in the emulator.

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
| Crash after adding data | ROM overflow into bank switching problems → section 10, [banking.md](banking.md) |
| `?ASlink-Warning-Undefined Global` | Missing `.c` file, typo, or declared in `assets.h` but never defined |
| Build error "multiple definition" | Two `.c` files with the same base name, data defined in a header, or `sfx_menu` defined outside `font_main.c` |
| Link error undefined `_scene_title` | Template with no game yet: write `src/scenes/title.c` (example in `src/scenes/file.txt`) |
| Converted sprite/map colors look off | Source PNG was upscaled without `scale:` in its notes, or has anti-aliased/blurred edges (more colors than expected) |
