#include "engine/engine.h"

/* Bonus run theme: E minor, 150 BPM (fpt 6), 4-bar loop = 64 ticks (drums: 32-tick loop).
   Chords: Em | C | D | B
   CH1 is a fake echo of the melody, one 16th behind (INST_ECHO). */

static const uint8_t ch1[] = {           /* echo */
    INST(INST_ECHO),
    R(L16),                                              /* intro: delay 1 */
    MUS_LOOP_POINT,
    N(E5,L8), N(G5,L8), N(B5,L8), N(G5,L8), N(A5,L8), N(G5,L8), N(E5,L4),   /* bar 1: 16 */
    N(E5,L8), N(G5,L8), N(C6,L4), N(B5,L8), N(A5,L8), N(G5,L4),             /* bar 2: 16 */
    N(Fs5,L8), N(A5,L8), N(D6,L4), N(C6,L8), N(B5,L8), N(A5,L4),            /* bar 3: 16 */
    N(B5,L4D), N(A5,L8), N(G5,L8), N(Fs5,L8), N(Ds5,L4),                    /* bar 4: 16 */
    MUS_LOOP
};

static const uint8_t ch2[] = {           /* melody */
    INST(INST_THIN), MUS_LOOP_POINT,
    N(E5,L8), N(G5,L8), N(B5,L8), N(G5,L8), N(A5,L8), N(G5,L8), N(E5,L4),   /* bar 1: 16 */
    N(E5,L8), N(G5,L8), N(C6,L4), N(B5,L8), N(A5,L8), N(G5,L4),             /* bar 2: 16 */
    N(Fs5,L8), N(A5,L8), N(D6,L4), N(C6,L8), N(B5,L8), N(A5,L4),            /* bar 3: 16 */
    N(B5,L4D), N(A5,L8), N(G5,L8), N(Fs5,L8), N(Ds5,L4),                    /* bar 4: 16 */
    MUS_LOOP
};

static const uint8_t ch3[] = {           /* bass: saw for bars 1-2, hollow square for bars 3-4 */
    MUS_LOOP_POINT,
    INST(WAVE_SAW),
    N(E2,L8), N(E3,L8), N(E2,L8), N(E3,L8), N(E2,L8), N(E3,L8), N(E2,L8), N(E3,L8),   /* bar 1: 16 */
    N(C2,L8), N(C3,L8), N(C2,L8), N(C3,L8), N(C2,L8), N(C3,L8), N(C2,L8), N(C3,L8),   /* bar 2: 16 */
    INST(WAVE_SQUARE),
    N(D2,L8), N(D3,L8), N(D2,L8), N(D3,L8), N(D2,L8), N(D3,L8), N(D2,L8), N(D3,L8),   /* bar 3: 16 */
    N(B2,L8), N(Fs3,L8), N(B2,L8), N(Fs3,L8), N(B2,L8), N(A2,L8), N(B2,L8), N(Ds3,L8), /* bar 4: 16 */
    MUS_LOOP
};

static const uint8_t ch4[] = {           /* drums */
    MUS_LOOP_POINT,
    D(KICK,L8), D(HAT,L8), D(SNARE,L8), D(HAT,L8), D(KICK,L8), D(KICK,L8), D(SNARE,L8), D(OHAT,L8),      /* bar A: 16 */
    D(KICK,L8), D(HAT,L8), D(SNARE,L8), D(HAT,L8), D(TOM,L16), D(TOM,L16), D(TOM,L8),
    D(SNARE,L16), D(SNARE,L16), D(CRASH,L8),                                                            /* fill: 16 */
    MUS_LOOP
};

const song_t mus_bonus = { 6, { ch1, ch2, ch3, ch4 } };
