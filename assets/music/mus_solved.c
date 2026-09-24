#include "engine/engine.h"

/* "Puzzle solved" fanfare: C major, 150 BPM (fpt 6), plays once (32 ticks per channel). */

static const uint8_t ch1[] = {
    INST(INST_SQUARE),
    N(E4,L4), N(G4,L4), N(C5,L4), N(E5,L4),              /* bar 1: 16 */
    N(G4,L4), N(C5,L4), N(E5,L2),                        /* bar 2: 16 */
    MUS_END
};

static const uint8_t ch2[] = {
    INST(INST_LEAD),
    N(C5,L8), N(E5,L8), N(G5,L8), N(C6,L8), N(E6,L4), N(C6,L4),              /* bar 1: 16 */
    N(G5,L8), N(C6,L8), N(E6,L8), N(G6,L8), N(C7,L2),                        /* bar 2: 16 */
    MUS_END
};

static const uint8_t ch3[] = {
    INST(WAVE_TRI),
    N(C3,L4), N(G2,L4), N(C3,L4), N(G2,L4),              /* bar 1: 16 */
    N(E3,L4), N(G3,L4), N(C3,L2),                        /* bar 2: 16 */
    MUS_END
};

static const uint8_t ch4[] = {
    D(CRASH,L2), D(KICK,L4), D(SNARE,L4),                /* bar 1: 16 */
    D(KICK,L8), D(KICK,L8), D(SNARE,L4), D(CRASH,L2),    /* bar 2: 16 */
    MUS_END
};

const song_t mus_solved = { 6, { ch1, ch2, ch3, ch4 } };
