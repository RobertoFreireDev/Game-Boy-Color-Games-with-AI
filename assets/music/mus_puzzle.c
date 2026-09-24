#include "engine/engine.h"

/* Puzzle (in-game) theme: A minor, calm, 100 BPM (fpt 9), 8-bar loop = 128 ticks.
   Chords: Am | F | C | G | Am | F | G | Am */

static const uint8_t ch1[] = {           /* harmony, long notes */
    INST(INST_SOFT), MUS_LOOP_POINT,
    N(C4,L2), N(E4,L2),                  /* bar 1: 16 */
    N(A3,L2), N(C4,L2),                  /* bar 2: 16 */
    N(E4,L2), N(G4,L2),                  /* bar 3: 16 */
    N(D4,L2), N(B3,L2),                  /* bar 4: 16 */
    N(C4,L2), N(E4,L2),                  /* bar 5: 16 */
    N(A3,L2), N(C4,L2),                  /* bar 6: 16 */
    N(B3,L2), N(D4,L2),                  /* bar 7: 16 */
    N(C4,L2), N(A3,L2),                  /* bar 8: 16 */
    MUS_LOOP
};

static const uint8_t ch2[] = {           /* melody */
    INST(INST_SOFT), MUS_LOOP_POINT,
    N(A4,L4), N(C5,L4), N(E5,L4D), N(D5,L8),             /* bar 1: 16 */
    N(C5,L4), N(A4,L4), N(F4,L2),                        /* bar 2: 16 */
    N(G4,L4), N(C5,L4), N(E5,L4), N(G5,L4),              /* bar 3: 16 */
    N(F5,L4D), N(E5,L8), N(D5,L2),                       /* bar 4: 16 */
    N(A4,L4), N(C5,L4), N(E5,L4D), N(A5,L8),             /* bar 5: 16 */
    N(G5,L4), N(F5,L4), N(E5,L4), N(C5,L4),              /* bar 6: 16 */
    N(D5,L4), N(E5,L4), N(G5,L4), N(D5,L4),              /* bar 7: 16 */
    N(E5,L2D), R(L4),                                    /* bar 8: 16 */
    MUS_LOOP
};

static const uint8_t ch3[] = {           /* bass */
    INST(WAVE_TRI), MUS_LOOP_POINT,
    N(A2,L4), N(E3,L4), N(A2,L4), N(E3,L4),              /* bar 1: 16 */
    N(F2,L4), N(C3,L4), N(F2,L4), N(C3,L4),              /* bar 2: 16 */
    N(C3,L4), N(G2,L4), N(C3,L4), N(G2,L4),              /* bar 3: 16 */
    N(G2,L4), N(D3,L4), N(G2,L4), N(D3,L4),              /* bar 4: 16 */
    N(A2,L4), N(E3,L4), N(A2,L4), N(E3,L4),              /* bar 5: 16 */
    N(F2,L4), N(C3,L4), N(F2,L4), N(C3,L4),              /* bar 6: 16 */
    N(G2,L4), N(D3,L4), N(G2,L4), N(D3,L4),              /* bar 7: 16 */
    N(A2,L2D), R(L4),                                    /* bar 8: 16 */
    MUS_LOOP
};

static const uint8_t ch4[] = {           /* soft drums: 1 bar = 16, divides 128 */
    MUS_LOOP_POINT,
    D(KICK,L8), R(L8), D(HAT,L4), D(SNARE,L4), D(HAT,L4),
    MUS_LOOP
};

const song_t mus_puzzle = { 9, { ch1, ch2, ch3, ch4 } };
