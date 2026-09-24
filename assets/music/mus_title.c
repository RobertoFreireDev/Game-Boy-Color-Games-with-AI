#include "engine/engine.h"

/* Title theme: C major, 150 BPM (fpt 6), 8-bar loop = 128 ticks per channel.
   Chords: C | F | Dm | G | C | G | C-G | C */

static const uint8_t ch1[] = {           /* harmony */
    INST(INST_SOFT), MUS_LOOP_POINT,
    N(E4,L4), N(G4,L4), N(E4,L4), N(G4,L4),              /* bar 1: 16 */
    N(F4,L4), N(A4,L4), N(F4,L4), N(A4,L4),              /* bar 2: 16 */
    N(F4,L4), N(A4,L4), N(D4,L4), N(A4,L4),              /* bar 3: 16 */
    N(D4,L4), N(G4,L4), N(B4,L4), N(G4,L4),              /* bar 4: 16 */
    N(E4,L4), N(G4,L4), N(E4,L4), N(G4,L4),              /* bar 5: 16 */
    N(D4,L4), N(G4,L4), N(B4,L4), N(G4,L4),              /* bar 6: 16 */
    N(E4,L4), N(G4,L4), N(D4,L4), N(F4,L4),              /* bar 7: 16 */
    N(E4,L2D), R(L4),                                    /* bar 8: 16 */
    MUS_LOOP
};

static const uint8_t ch2[] = {           /* melody */
    INST(INST_LEAD), MUS_LOOP_POINT,
    N(C5,L8), N(E5,L8), N(G5,L8), N(C6,L8), N(B5,L4), N(G5,L4),              /* bar 1: 16 */
    N(A5,L4), N(F5,L8), N(A5,L8), N(G5,L2),                                  /* bar 2: 16 */
    N(F5,L8), N(E5,L8), N(D5,L8), N(E5,L8), N(F5,L4), N(A5,L4),              /* bar 3: 16 */
    N(G5,L4D), N(F5,L8), N(E5,L4), N(D5,L4),                                 /* bar 4: 16 */
    N(C5,L8), N(E5,L8), N(G5,L8), N(C6,L8), N(D6,L4), N(C6,L4),              /* bar 5: 16 */
    N(B5,L8), N(A5,L8), N(G5,L8), N(A5,L8), N(B5,L2),                        /* bar 6: 16 */
    N(C6,L8), N(G5,L8), N(E5,L8), N(G5,L8), N(F5,L8), N(D5,L8), N(B4,L8), N(D5,L8), /* bar 7: 16 */
    N(C5,L2D), R(L4),                                                        /* bar 8: 16 */
    MUS_LOOP
};

static const uint8_t ch3[] = {           /* bass */
    INST(WAVE_TRI), MUS_LOOP_POINT,
    N(C3,L4), N(G2,L4), N(C3,L4), N(G2,L4),              /* bar 1: 16 */
    N(F2,L4), N(C3,L4), N(F2,L4), N(C3,L4),              /* bar 2: 16 */
    N(D3,L4), N(A2,L4), N(D3,L4), N(A2,L4),              /* bar 3: 16 */
    N(G2,L4), N(D3,L4), N(G2,L4), N(D3,L4),              /* bar 4: 16 */
    N(C3,L4), N(G2,L4), N(C3,L4), N(G2,L4),              /* bar 5: 16 */
    N(G2,L4), N(D3,L4), N(G2,L4), N(D3,L4),              /* bar 6: 16 */
    N(C3,L4), N(E3,L4), N(G2,L4), N(B2,L4),              /* bar 7: 16 */
    N(C3,L2D), R(L4),                                    /* bar 8: 16 */
    MUS_LOOP
};

static const uint8_t ch4[] = {           /* drums: 1 bar = 16, divides 128 */
    MUS_LOOP_POINT,
    D(KICK,L4), D(HAT,L8), D(HAT,L8), D(SNARE,L4), D(HAT,L8), D(HAT,L8),
    MUS_LOOP
};

const song_t mus_title = { 6, { ch1, ch2, ch3, ch4 } };
