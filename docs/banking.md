# ROM banking

Read when: the linker reports ROM/area overflow (non-banked code + data past 32 KB).

## 10. ROM banking (only when the game grows past 32 KB)

The linker prints an error about ROM/area overflow when non-banked code+data exceed 32 KB. Then:
1. Keep **all code, music, SFX and font non-banked**.
2. Move large **maps, tilesets and sprite data** to banked files: add `#pragma bank 255` as the first line and `BANKREF(name)` for the main struct; in `assets.h` use `BANKREF_EXTERN(name)`.
3. Everything a load call touches must be **in the same banked file** (a map file includes its own tileset rows/tiles/palettes; a sprite file its tiles and palette).
4. Load with the bank: `map_load(&map_level5, BANK(map_level5));` `gfx_load_sprite(&h, &spr_boss, BANK(spr_boss));`
5. Engine loaders switch and restore: `uint8_t save = CURRENT_BANK; if (bank) SWITCH_ROM(bank); ... if (bank) SWITCH_ROM(save);`. The map module stores the bank and switches in `map_char` and while streaming.
