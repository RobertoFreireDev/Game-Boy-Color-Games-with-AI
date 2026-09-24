#include "engine/engine.h"

static uint8_t spr_vram_next;

static void set_pal(uint8_t idx, const palette_color_t *c4) {
    pal_ram[idx]     = c4[0];
    pal_ram[idx + 1] = c4[1];
    pal_ram[idx + 2] = c4[2];
    pal_ram[idx + 3] = c4[3];
    fade_apply();
}

void gfx_set_bkg_palette(uint8_t slot, const palette_color_t *c4) { set_pal(slot << 2, c4); }
void gfx_set_obj_palette(uint8_t slot, const palette_color_t *c4) { set_pal(32 + (slot << 2), c4); }

uint8_t gfx_load_sprite(sprite_t *out, const sprite_def_t *def, uint8_t bank) {
    uint8_t save = CURRENT_BANK;
    uint8_t tpf, count, ok = 1;
    if (bank) SWITCH_ROM(bank);
    tpf = def->w * def->h;
    count = tpf * def->frames;
    if ((uint16_t)spr_vram_next + count > 128) {
        ok = 0;
    } else {
        VBK_REG = 0;
        set_sprite_data(spr_vram_next, count, def->tiles);
        out->base = spr_vram_next;
        out->w = def->w; out->h = def->h;
        out->tpf = tpf; out->frames = def->frames;
        out->pal = def->pal_slot;
        gfx_set_obj_palette(def->pal_slot, def->pal);
        spr_vram_next += count;
    }
    if (bank) SWITCH_ROM(save);
    return ok;
}

void gfx_reset(void) { spr_vram_next = 0; }
