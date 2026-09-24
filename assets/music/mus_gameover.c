#include "engine/engine.h"

/* Game over: A minor, slow, 90 BPM (fpt 10), plays once (32 ticks per channel). */

static const uint8_t ch1[] = {           /* harmony */
    INST(INST_SOFT),
    N(C4,L2), N(A3,L2),                                  /* bar 1: 16 */
    N(F3,L2), N(E3,L2),                                  /* bar 2: 16 */
    MUS_END
};

static const uint8_t ch2[] = {           /* melody, plucked */
    INST(INST_PLUCK),
    N(A4,L4), N(G4,L4), N(E4,L4), N(C4,L4),              /* bar 1: 16 */
    N(D4,L4), N(B3,L4), N(A3,L2),                        /* bar 2: 16 */
    MUS_END
};

static const uint8_t ch3[] = {           /* bass */
    INST(WAVE_SOFTTRI),
    N(A2,L2), N(E2,L2),                                  /* bar 1: 16 */
    N(F2,L2), N(A2,L2),                                  /* bar 2: 16 */
    MUS_END
};

static const uint8_t ch4[] = {
    D(TOM,L4), R(L4), D(TOM,L4), R(L4),                  /* bar 1: 16 */
    D(KICK,L2), D(CRASH,L2),                             /* bar 2: 16 */
    MUS_END
};

const song_t mus_gameover = { 10, { ch1, ch2, ch3, ch4 } };
