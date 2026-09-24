#include "engine/engine.h"
#include <string.h>
#include "game/state.h"

#define SAVE_MAGIC 0x5B0C

typedef struct {
    uint16_t magic;
    uint8_t unlocked;
    uint16_t best[LEVEL_COUNT];
    uint8_t check;
} save_t;

uint8_t game_level, game_unlocked;
uint16_t game_best[LEVEL_COUNT];
uint16_t game_moves, game_pushes;
uint8_t game_new_best;

static save_t save_buf;

static uint8_t save_checksum(const save_t *s) {
    const uint8_t *p = (const uint8_t *)s;
    uint8_t i, sum = 0x5A;
    for (i = 0; i < sizeof(save_t) - 1; i++) sum += p[i];
    return sum;
}

void save_load(void) {
    uint8_t i;
    ENABLE_RAM; SWITCH_RAM(0);
    memcpy(&save_buf, (const void *)0xA000, sizeof(save_t));
    DISABLE_RAM;
    game_unlocked = 0;
    for (i = 0; i < LEVEL_COUNT; i++) game_best[i] = 0;
    if (save_buf.magic != SAVE_MAGIC || save_buf.check != save_checksum(&save_buf)) return;
    game_unlocked = MIN(save_buf.unlocked, LEVEL_COUNT - 1);
    for (i = 0; i < LEVEL_COUNT; i++) game_best[i] = save_buf.best[i];
}

static void save_write(void) {
    uint8_t i;
    save_buf.magic = SAVE_MAGIC;
    save_buf.unlocked = game_unlocked;
    for (i = 0; i < LEVEL_COUNT; i++) save_buf.best[i] = game_best[i];
    save_buf.check = save_checksum(&save_buf);
    ENABLE_RAM; SWITCH_RAM(0);
    memcpy((void *)0xA000, &save_buf, sizeof(save_t));
    DISABLE_RAM;
}

void game_record_solve(void) {
    uint16_t *b = &game_best[game_level];
    game_new_best = (*b == 0 || game_moves < *b);
    if (game_new_best) *b = game_moves;
    if (game_level + 1 < LEVEL_COUNT && game_unlocked <= game_level) game_unlocked = game_level + 1;
    save_write();
}
