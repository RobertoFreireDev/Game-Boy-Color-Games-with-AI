#include "engine/engine.h"

static const uint16_t note_freq[72] = {
    44, 157, 263, 363, 457, 547, 631, 711, 786, 856, 923, 986,               /* 2 */
    1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517, /* 3 */
    1547, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783, /* 4 */
    1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915, /* 5 */
    1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964, 1969, 1974, 1978, 1982, /* 6 */
    1985, 1989, 1992, 1995, 1998, 2001, 2004, 2006, 2009, 2011, 2013, 2015, /* 7 */
};

static const uint8_t pulse_inst[][2] = {     /* duty (NRx1), envelope (NRx2) */
    {0x80, 0xC4}, {0x80, 0xA0}, {0x00, 0xA3}, {0x40, 0xF1}, {0x40, 0x75}, {0x80, 0x42} };

static const uint8_t wave_inst[][2] = {      /* wave index, NR32 volume (0x20 full, 0x40 half) */
    {0, 0x20}, {1, 0x40}, {2, 0x40}, {0, 0x40} };
static const uint8_t waves[3][16] = {
    {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10},  /* triangle */
    {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF},  /* saw */
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* square */
};

static const uint8_t drums[][2] = {          /* envelope (NR42), poly (NR43) */
    {0xF1, 0x6D}, {0xD2, 0x42}, {0x81, 0x00}, {0x83, 0x00}, {0xF5, 0x21}, {0xC2, 0x5D} };

/* music state */
static const song_t *song;
static uint8_t tick_cnt;
static const uint8_t *mpos[4], *mloop[4];
static uint8_t mwait[4], minst[4], mactive[4];
static uint8_t cur_wave;

/* sfx state: index 0 = CH1, 1 = CH4 */
static const uint8_t *spos[2];
static uint8_t swait[2], sprio[2], son[2];

#define OWNED(ch) (((ch) == 0 && son[0]) || ((ch) == 3 && son[1]))

static void silence(uint8_t ch) {
    switch (ch) {
    case 0: NR12_REG = 0; break;
    case 1: NR22_REG = 0; break;
    case 2: NR30_REG = 0; break;
    default: NR42_REG = 0; break;
    }
}

static void play_note(uint8_t ch, uint8_t note) {
    uint16_t f;
    uint8_t i, w;
    switch (ch) {
    case 0:
        f = note_freq[note];
        NR10_REG = 0; NR11_REG = pulse_inst[minst[0]][0]; NR12_REG = pulse_inst[minst[0]][1];
        NR13_REG = (uint8_t)f; NR14_REG = 0x80 | (uint8_t)(f >> 8);
        break;
    case 1:
        f = note_freq[note];
        NR21_REG = pulse_inst[minst[1]][0]; NR22_REG = pulse_inst[minst[1]][1];
        NR23_REG = (uint8_t)f; NR24_REG = 0x80 | (uint8_t)(f >> 8);
        break;
    case 2:
        f = note_freq[MIN(note + 12, 71)];
        w = wave_inst[minst[2]][0];
        if (w != cur_wave) {
            NR30_REG = 0;
            for (i = 0; i < 16; i++) ((volatile uint8_t *)0xFF30)[i] = waves[w][i];
            cur_wave = w;
        }
        NR30_REG = 0x80; NR32_REG = wave_inst[minst[2]][1];
        NR33_REG = (uint8_t)f; NR34_REG = 0x80 | (uint8_t)(f >> 8);
        break;
    default:
        NR42_REG = drums[note][0]; NR43_REG = drums[note][1]; NR44_REG = 0x80;
        break;
    }
}

static void music_tick(uint8_t ch) {
    const uint8_t *p;
    uint8_t c;
    if (!mactive[ch]) return;
    if (mwait[ch] > 1) { mwait[ch]--; return; }
    p = mpos[ch];
    for (;;) {
        c = *p++;
        if (c == MUS_END) { mactive[ch] = 0; if (!OWNED(ch)) silence(ch); break; }
        if (c == MUS_LOOP) { p = mloop[ch]; continue; }
        if (c == MUS_LOOP_POINT) { mloop[ch] = p; continue; }
        if (c == 0xF1) { minst[ch] = *p++; continue; }
        mwait[ch] = *p++;
        if (!OWNED(ch)) { if (c == 0xF0) silence(ch); else play_note(ch, c); }
        break;
    }
    mpos[ch] = p;
}

static void sfx_step(uint8_t i) {
    const uint8_t *p;
    uint16_t f;
    if (!son[i]) return;
    if (swait[i]) { swait[i]--; if (swait[i]) return; }
    p = spos[i];
    if (*p == SFX_END) { son[i] = 0; silence(i ? 3 : 0); return; }
    swait[i] = *p++;
    if (i == 0) {
        NR10_REG = p[0]; NR11_REG = p[1]; NR12_REG = p[2];
        f = note_freq[p[3]];
        NR13_REG = (uint8_t)f; NR14_REG = 0x80 | (uint8_t)(f >> 8);
        p += 4;
    } else {
        NR42_REG = p[0]; NR43_REG = p[1]; NR44_REG = 0x80;
        p += 2;
    }
    spos[i] = p;
}

void audio_update(void) {
    uint8_t ch;
    if (song && ++tick_cnt >= song->fpt) {
        tick_cnt = 0;
        for (ch = 0; ch < 4; ch++) music_tick(ch);
    }
    sfx_step(0);
    sfx_step(1);
}

void audio_init(void) {
    NR52_REG = 0x80;
    NR50_REG = 0x77;
    NR51_REG = 0xFF;
    cur_wave = 0xFF;
    CRITICAL { add_VBL(audio_update); }
}

void music_play(const song_t *s) {
    uint8_t ch;
    CRITICAL {
        song = s;
        tick_cnt = s->fpt - 1;           /* first tick on the next frame */
        for (ch = 0; ch < 4; ch++) {
            mpos[ch] = mloop[ch] = s->ch[ch];
            mwait[ch] = 0; minst[ch] = 0;
            mactive[ch] = s->ch[ch] != 0;
            if (!OWNED(ch)) silence(ch);
        }
    }
}

void music_stop(void) {
    uint8_t ch;
    CRITICAL {
        song = 0;
        for (ch = 0; ch < 4; ch++) { mactive[ch] = 0; if (!OWNED(ch)) silence(ch); }
    }
}

void sfx_play(const sfx_t *s) {
    uint8_t i = (s->ch == SFX_CH1) ? 0 : 1;
    if (son[i] && s->prio < sprio[i]) return;
    CRITICAL {
        spos[i] = s->data; swait[i] = 0; sprio[i] = s->prio; son[i] = 1;
    }
}
