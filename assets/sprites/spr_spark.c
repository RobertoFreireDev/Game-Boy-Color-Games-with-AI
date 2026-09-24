#include "engine/engine.h"

/* 0 = transparent, 1 = orange, 2 = yellow, 3 = white */
const palette_color_t pal_spark[4] = { RGB8(0,0,0), RGB8(232,112,32), RGB8(248,216,64), RGB8(255,255,240) };

/* Sparkle / particle, 8x8, 3 frames shrinking over its lifetime: 0 star, 1 small star, 2 dot.
   Centre pixel is (3,3) in every frame. Also used for the twinkling title stars. */
const uint8_t spr_spark_tiles[] = {
    /* frame 0: star */
    PX(0,0,0,1,0,0,0,0),
    PX(0,0,0,2,0,0,0,0),
    PX(0,0,1,3,1,0,0,0),
    PX(1,2,3,3,3,2,1,0),
    PX(0,0,1,3,1,0,0,0),
    PX(0,0,0,2,0,0,0,0),
    PX(0,0,0,1,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    /* frame 1: small star */
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,2,0,0,0,0),
    PX(0,0,2,3,2,0,0,0),
    PX(0,0,0,2,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    /* frame 2: dot */
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,1,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
    PX(0,0,0,0,0,0,0,0),
};

const sprite_def_t spr_spark = { spr_spark_tiles, 1, 1, 3, pal_spark, 6 };
