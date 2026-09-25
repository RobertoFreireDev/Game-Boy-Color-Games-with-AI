# Music, sound effects and the audio driver

Read when: writing music or SFX, or touching `src/engine/audio.c`.

## 6. Asset formats (audio part)

### 6.7 Music — `assets/music/mus_theme.c`

Each channel is a byte stream of commands. Time unit = **tick** = one 16th note. Song tempo = frames per tick (`fpt`).

```c
typedef struct { uint8_t fpt; const uint8_t *ch[4]; } song_t;   /* ch[i] may be 0 (unused) */

/* commands (audio.h) */
#define N(note,len)   NOTE_##note, (len)      /* play note on CH1/CH2/CH3 */
#define D(drum,len)   DRUM_##drum, (len)      /* play drum on CH4 */
#define R(len)        0xF0, (len)             /* rest (silence) */
#define INST(i)       0xF1, (i)               /* change instrument */
#define MUS_LOOP_POINT 0xFD                   /* loop returns here (default: start) */
#define MUS_LOOP       0xFE                   /* jump to loop point */
#define MUS_END        0xFF                   /* stop channel */

/* lengths in ticks */
#define L1 16
#define L2D 12
#define L2 8
#define L4D 6
#define L4 4
#define L8D 3
#define L8 2
#define L16 1
```
Notes: `C2 Cs2 D2 Ds2 E2 F2 Fs2 G2 Gs2 A2 As2 B2` … up to `B7` (`s` = sharp). Use flats as the sharp below (Bb4 = `As4`). CH3 plays at written pitch (engine compensates).

Tempo: `BPM ≈ 900 / fpt` → fpt 5 = 180 BPM, 6 = 150, 7 = 128, 8 = 112, 10 = 90.

```c
#include "engine/engine.h"

static const uint8_t ch1[] = {           /* harmony (SFX may interrupt this channel) */
    INST(INST_SOFT), MUS_LOOP_POINT,
    N(G4,L4), N(D5,L4), N(C5,L4), N(B4,L4),
    N(A4,L4), N(G4,L4), N(B4,L2),
    MUS_LOOP
};
static const uint8_t ch2[] = {           /* melody */
    INST(INST_LEAD), MUS_LOOP_POINT,
    N(E5,L8), N(G5,L8), N(C6,L4), N(B5,L8), N(G5,L8), N(A5,L4),
    N(G5,L8), N(E5,L8), N(F5,L8), N(D5,L8), N(C5,L2),
    MUS_LOOP
};
static const uint8_t ch3[] = {           /* bass */
    INST(WAVE_TRI), MUS_LOOP_POINT,
    N(C3,L4), N(G3,L4), N(A3,L4), N(E3,L4),
    N(F3,L4), N(C3,L4), N(G3,L2),
    MUS_LOOP
};
static const uint8_t ch4[] = {           /* drums */
    MUS_LOOP_POINT,
    D(KICK,L8), D(HAT,L8), D(SNARE,L8), D(HAT,L8),
    MUS_LOOP
};
const song_t mus_theme = { 7, { ch1, ch2, ch3, ch4 } };
```
**Rule:** the looped part of every channel must have the same total length in ticks, or a length that divides it (here 32, 32, 32, 8). Always count ticks in a comment per bar.

Instruments (engine table, pick by name):
- CH1/CH2 pulse: `INST_LEAD` (50% duty, medium decay), `INST_SQUARE` (50%, sustained), `INST_THIN` (12.5%, bright), `INST_PLUCK` (25%, short), `INST_SOFT` (25%, quiet), `INST_ECHO` (50%, very quiet — use for fake echo a 16th behind the melody).
- CH3 wave: `WAVE_TRI` (round bass), `WAVE_SAW` (buzzy), `WAVE_SQUARE` (hollow), `WAVE_SOFTTRI` (quiet).
- CH4 drums: `KICK`, `SNARE`, `HAT`, `OHAT`, `CRASH`, `TOM`.

### 6.8 Sound effects — `assets/sfx/sfx_all.c`

An SFX takes over **CH1** (tones, has pitch sweep) or **CH4** (noise) for a few frames; music on that channel is muted meanwhile and resumes at its next note.

```c
typedef struct { uint8_t ch; uint8_t prio; const uint8_t *data; } sfx_t;   /* ch: SFX_CH1 or SFX_CH4 */

/* steps (audio.h) — each step lasts `frames` frames (1/60 s) */
#define SFX_TONE(frames, sweep, duty, env, note) (frames), (sweep), (duty), (env), NOTE_##note
#define SFX_NOISE(frames, env, poly)             (frames), (env), (poly)
#define SFX_END 0
```
Register cheat sheet:
- `sweep` (NR10): `0x00` none. `0bTTTDSSS`: T = time 1–7 (slower as it grows), D = 0 up / 1 down, S = shift 1–7 (bigger = smaller change). Up: `0x15`, `0x26`; down: `0x1D`, `0x2E`.
- `duty` (NR11): `0x00` 12.5% · `0x40` 25% · `0x80` 50% · `0xC0` 75%.
- `env` (NR12/NR42): `0xVDP`… high nibble = start volume 0–F, bit 3 = 1 grow / 0 fade, low 3 bits = speed (1 fast … 7 slow, 0 = hold). `0xF1` loud short blip, `0xF3` loud medium, `0xA7` long fade, `0x81` quiet tick.
- `poly` (NR43): high nibble = pitch shift (0 = hiss, F = rumble), bit 3 = 1 metallic/tonal, low 3 bits = divider. `0x00` hiss, `0x40` crunch, `0x6D` thump, `0x74` explosion rumble.

```c
#include "engine/engine.h"
static const uint8_t d_jump[]  = { SFX_TONE(10, 0x15, 0x80, 0xF3, A4), SFX_END };
static const uint8_t d_coin[]  = { SFX_TONE(4, 0x00, 0x80, 0xF1, B5), SFX_TONE(12, 0x00, 0x80, 0xF3, E6), SFX_END };
static const uint8_t d_hurt[]  = { SFX_TONE(12, 0x2E, 0x40, 0xF2, E5), SFX_END };
static const uint8_t d_hit[]   = { SFX_NOISE(8, 0xF1, 0x40), SFX_END };
static const uint8_t d_boom[]  = { SFX_NOISE(4, 0xF1, 0x20), SFX_NOISE(40, 0xF7, 0x74), SFX_END };
const sfx_t sfx_jump = { SFX_CH1, 1, d_jump };
const sfx_t sfx_coin = { SFX_CH1, 2, d_coin };
const sfx_t sfx_hurt = { SFX_CH1, 3, d_hurt };
const sfx_t sfx_hit  = { SFX_CH4, 2, d_hit };
const sfx_t sfx_boom = { SFX_CH4, 3, d_boom };
```
A new SFX replaces the one playing on the same channel only if its `prio` is ≥ the current one. `sfx_menu` is already defined in `font_main.c` (6.6, [graphics.md](graphics.md)); defining it here too is a "multiple definition" link error.

## 7. Engine API (audio part)

### 7.13 audio

```c
void audio_init(void);          /* NR52 = 0x80; NR50 = 0x77; NR51 = 0xFF; CRITICAL { add_VBL(audio_update); } */
void audio_update(void);        /* VBL interrupt: music tick + sfx step */
void music_play(const song_t *s);   /* wrap state changes in CRITICAL { } */
void music_stop(void);
void sfx_play(const sfx_t *s);
```
Frequency table (register value = 2048 − 131072 / Hz), index 0 = C2 … 71 = B7:
```c
static const uint16_t note_freq[72] = {
    44, 157, 263, 363, 457, 547, 631, 711, 786, 856, 923, 986,               /* 2 */
    1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517, /* 3 */
    1547, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783, /* 4 */
    1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915, /* 5 */
    1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964, 1969, 1974, 1978, 1982, /* 6 */
    1985, 1989, 1992, 1995, 1998, 2001, 2004, 2006, 2009, 2011, 2013, 2015, /* 7 */
};
```
Note enum in audio.h, in this order: `NOTE_C2, NOTE_Cs2, NOTE_D2, NOTE_Ds2, NOTE_E2, NOTE_F2, NOTE_Fs2, NOTE_G2, NOTE_Gs2, NOTE_A2, NOTE_As2, NOTE_B2, NOTE_C3, …, NOTE_B7` (72 values, 0–71).

Instrument tables:
```c
enum { INST_LEAD, INST_SQUARE, INST_THIN, INST_PLUCK, INST_SOFT, INST_ECHO };
static const uint8_t pulse_inst[][2] = {     /* duty (NRx1), envelope (NRx2) */
    {0x80, 0xC4}, {0x80, 0xA0}, {0x00, 0xA3}, {0x40, 0xF1}, {0x40, 0x75}, {0x80, 0x42} };

enum { WAVE_TRI, WAVE_SAW, WAVE_SQUARE, WAVE_SOFTTRI };
static const uint8_t wave_inst[][2] = {      /* wave index, NR32 volume (0x20 full, 0x40 half) */
    {0, 0x20}, {1, 0x40}, {2, 0x40}, {0, 0x40} };
static const uint8_t waves[3][16] = {
    {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10},  /* triangle */
    {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF},  /* saw */
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* square */
};

enum { DRUM_KICK, DRUM_SNARE, DRUM_HAT, DRUM_OHAT, DRUM_CRASH, DRUM_TOM };
static const uint8_t drums[][2] = {          /* envelope (NR42), poly (NR43) */
    {0xF1, 0x6D}, {0xD2, 0x42}, {0x81, 0x00}, {0x83, 0x00}, {0xF5, 0x21}, {0xC2, 0x5D} };
```
Driver rules:
- Per channel state: `pos`, `loop`, `wait`, `inst`. Every `fpt` frames = one tick. When `wait` reaches 0, read commands until a note/drum/rest, then `wait = len`.
- Note trigger CH1: `NR10 = 0; NR11 = duty; NR12 = env; NR13 = f & 0xFF; NR14 = 0x80 | (f >> 8)`. CH2: `NR21..NR24` the same.
- CH3: use `note_freq[MIN(note + 12, 71)]` (CH3 sounds an octave lower). If the wave changed: `NR30 = 0`, copy 16 bytes to `0xFF30`–`0xFF3F`. Then `NR30 = 0x80; NR32 = vol; NR33 = f & 0xFF; NR34 = 0x80 | (f >> 8)`.
- CH4: `NR42 = env; NR43 = poly; NR44 = 0x80`.
- Rest: CH1 `NR12 = 0`, CH2 `NR22 = 0`, CH3 `NR30 = 0`, CH4 `NR42 = 0`.
- While an SFX owns a channel, music still advances on it but skips all register writes.
- SFX step: CH1 `NR10 = sweep; NR11 = duty; NR12 = env; NR13/NR14` from the note table with trigger. CH4: `NR42 = env; NR43 = poly; NR44 = 0x80`. On `SFX_END`: silence the channel and release it.
- **Music and SFX data must not be banked** (the interrupt reads them at any time).

## 11. Composing music and SFX

- Channel roles: **CH2 melody**, **CH1 harmony/counter-melody** (SFX interrupt it), **CH3 bass**, **CH4 drums** (noise SFX interrupt it).
- Write in a key; keep melody in octaves 4–6, bass in octaves 2–3.
- Structure: 4- or 8-bar phrases, intro before `MUS_LOOP_POINT`, loop length 8–32 bars.
- Comment each bar with its tick total (`/* bar 3: 16 */`). Verify every channel's loop sums to the same length (or a divisor).
- Mood guide: title = catchy, 120–150 BPM; exploration = calm, 90–112 BPM, `INST_SOFT`; battle = fast 150–180 BPM, `INST_THIN` lead, busy drums; game over = slow, minor key, no loop (`MUS_END`).
- SFX: short (3–20 frames), higher priority for important feedback (hurt 3, coin 2, jump 1). Rising sweeps = positive (jump, power-up); falling = negative (hurt, lose).
