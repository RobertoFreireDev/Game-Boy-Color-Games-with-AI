#include "unit.h"

static uint16_t n_pass, n_fail;
static uint8_t cur_failed;
static const char *cur_name;

/* ---------- serial output (read by gbc.mjs) ---------- */

static void put_ch(char c) {
    SB_REG = (uint8_t)c;
    SC_REG = 0x81;
    while (SC_REG & 0x80) ;
}

void unit_puts(const char *s) { while (*s) put_ch(*s++); }

static void put_num(int32_t n) {
    char tmp[12];
    uint8_t i = 0;
    uint32_t u;
    if (n < 0) { put_ch('-'); u = (uint32_t)(-n); } else u = (uint32_t)n;
    do { tmp[i++] = (char)('0' + (uint8_t)(u % 10)); u /= 10; } while (u);
    while (i) put_ch(tmp[--i]);
}

void unit_fail(uint16_t line, const char *expr, int32_t got, int32_t want, uint8_t has_values) {
    if (!cur_failed) { unit_puts("FAIL "); unit_puts(cur_name); unit_puts("\n"); }
    cur_failed = 1;
    unit_puts("  line "); put_num(line); unit_puts(": "); unit_puts(expr);
    if (has_values) { unit_puts(" == "); put_num(got); unit_puts(", expected "); put_num(want); }
    unit_puts("\n");
}

void unit_run(void (*fn)(void), const char *name) {
    cur_name = name;
    cur_failed = 0;
    unit_setup();
    fn();
    if (cur_failed) n_fail++;
    else { n_pass++; unit_puts("ok   "); unit_puts(name); unit_puts("\n"); }
}

void unit_reset_engine(void) {
    scene_reset_screen();
    gfx_reset();
    tween_clear();
    particles_clear();
    cam_reset();
    keys = keys_prev = 0;
    pad_hold(0);
}

/* ---------- joypad mock ---------- */

uint16_t pad_reads;
static const uint8_t *pad_seq;
static uint8_t pad_len, pad_pos, pad_repeat, pad_const;

void pad_hold(uint8_t k) { pad_seq = 0; pad_const = k; pad_reads = 0; }
void pad_script(const uint8_t *k, uint8_t n) { pad_seq = k; pad_len = n; pad_pos = 0; pad_repeat = 0; pad_const = 0; pad_reads = 0; }
void pad_loop(const uint8_t *k, uint8_t n) { pad_script(k, n); pad_repeat = 1; }

uint8_t unit_pad_next(void) {
    uint8_t k;
    pad_reads++;
    if (!pad_seq) return pad_const;
    if (pad_pos >= pad_len) {
        if (!pad_repeat) return 0;
        pad_pos = 0;
    }
    k = pad_seq[pad_pos++];
    return k;
}

/* Replaces GBDK's joypad() (the linker takes this one, so pad.o is never pulled in).
   GBDK callers expect b, c, h, l preserved, so wrap the C function. */
uint8_t joypad(void) PRESERVES_REGS(b, c, h, l) NAKED {
    __asm
        push bc
        push hl
        call _unit_pad_next
        pop hl
        pop bc
        ret
    __endasm;
}

/* ---------- hardware readers ---------- */

uint16_t hw_bkg_color(uint8_t pal, uint8_t col) {
    uint8_t i = (uint8_t)((pal << 3) + (col << 1)), lo, hi;
    BCPS_REG = i;     lo = BCPD_REG;
    BCPS_REG = i + 1; hi = BCPD_REG;
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

uint16_t hw_obj_color(uint8_t pal, uint8_t col) {
    uint8_t i = (uint8_t)((pal << 3) + (col << 1)), lo, hi;
    OCPS_REG = i;     lo = OCPD_REG;
    OCPS_REG = i + 1; hi = OCPD_REG;
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

uint8_t hw_vram(uint8_t bank, uint16_t addr) {
    uint8_t v, save = VBK_REG & 1;
    VBK_REG = bank;
    v = *(volatile uint8_t *)addr;
    VBK_REG = save;
    return v;
}

static uint8_t map_read(uint8_t win, uint8_t bank, uint8_t x, uint8_t y) {
    uint8_t v, save = VBK_REG & 1;
    VBK_REG = bank;
    v = win ? get_win_tile_xy(x, y) : get_bkg_tile_xy(x, y);
    VBK_REG = save;
    return v;
}

uint8_t hw_bkg_tile(uint8_t x, uint8_t y) { return map_read(0, 0, x, y); }
uint8_t hw_bkg_attr(uint8_t x, uint8_t y) { return map_read(0, 1, x, y); }
uint8_t hw_win_tile(uint8_t x, uint8_t y) { return map_read(1, 0, x, y); }
uint8_t hw_win_attr(uint8_t x, uint8_t y) { return map_read(1, 1, x, y); }

/* ---------- entry point ---------- */

void main(void) {
    engine_init();
    unit_tests();
    unit_puts("DONE "); put_num(n_pass); unit_puts(" "); put_num(n_fail); unit_puts("\n");
    put_ch('\x04');
    while (1) vsync();
}
