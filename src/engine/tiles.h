#ifndef TILES_H
#define TILES_H
#include <stdint.h>
#define PXL_(a,b,c,d,e,f,g,h) ((uint8_t)((((a)&1)<<7)|(((b)&1)<<6)|(((c)&1)<<5)|(((d)&1)<<4)| \
                                          (((e)&1)<<3)|(((f)&1)<<2)|(((g)&1)<<1)|((h)&1)))
#define PXH_(a,b,c,d,e,f,g,h) PXL_((a)>>1,(b)>>1,(c)>>1,(d)>>1,(e)>>1,(f)>>1,(g)>>1,(h)>>1)
/* Game Boy 2bpp row = low bit-plane byte, then high bit-plane byte */
#define PX(a,b,c,d,e,f,g,h) PXL_(a,b,c,d,e,f,g,h), PXH_(a,b,c,d,e,f,g,h)
#define TILE_BYTES 16
#endif
