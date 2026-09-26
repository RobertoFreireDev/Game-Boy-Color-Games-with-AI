#ifndef DATA_BANKED_H
#define DATA_BANKED_H
#include "engine/engine.h"

/* 1x1 sprite, 2 frames, OBJ slot 5; tile bytes 0x01..0x10 then 0xF1..0xFF,0xEE; colors RGB(3,5,7) ... */
BANKREF_EXTERN(spr_banked)
extern const sprite_def_t spr_banked;

/* 20x18 room: '#' border (tile 1, TF_SOLID), '.' (tile 0), 'K' spawn at (8,2) */
BANKREF_EXTERN(map_banked)
extern const map_def_t map_banked;
#endif
