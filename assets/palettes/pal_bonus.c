#include "engine/engine.h"

/* BG palettes for the bonus run (ts_bonus). Bricks reuse pal_wh_wall (slot 0).
   Every other palette starts with the dark interior color, so index 0 blends into the back wall. */
const palette_color_t pal_bn_back[4]  = { RGB8(40,44,72),  RGB8(56,60,96),    RGB8(80,88,128),   RGB8(24,24,44) };
/* wood: shelves, crates, sign (1 light, 2 wood, 3 dark, same ramp as the crates) */
const palette_color_t pal_bn_wood[4]  = { RGB8(40,44,72),  RGB8(248,208,136), RGB8(208,136,64),  RGB8(88,40,24) };
const palette_color_t pal_bn_spike[4] = { RGB8(40,44,72),  RGB8(240,240,248), RGB8(160,168,192), RGB8(64,64,96) };
/* door: 0 = dark doorway (the keeper walks "into" it with SPR_BEHIND) */
const palette_color_t pal_bn_door[4]  = { RGB8(12,8,20),   RGB8(232,168,88),  RGB8(160,96,48),   RGB8(72,40,24) };
/* foreground fence (TF_OVER): bars cover sprites, gaps (0) let them show */
const palette_color_t pal_bn_fence[4] = { RGB8(40,44,72),  RGB8(200,208,224), RGB8(128,136,160), RGB8(56,56,80) };
