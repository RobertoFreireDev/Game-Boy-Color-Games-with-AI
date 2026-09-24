#include "engine/engine.h"
#include "game/board.h"

#define HIST_MAX 64            /* undo depth (ring buffer, power of 2) */

uint8_t board[BOARD_SIZE];
uint8_t player_cell;
const int8_t dir_off[4] = { -BOARD_W, BOARD_W, -1, 1 };

static uint8_t goal_count;
static uint8_t hist[HIST_MAX];
static uint8_t hist_head, hist_len;

/* map characters of a cell's 4 tiles (TL, TR, BL, BR), indexed by (state & (BOX|GOAL)) >> 1 */
static const char cell_chars[4][4] = {
    { '-', '-', '-', '-' },    /* floor */
    { 'a', 'b', 'c', 'd' },    /* goal */
    { 'B', 'C', 'D', 'E' },    /* crate */
    { 'W', 'X', 'Y', 'Z' },    /* crate on goal */
};

void board_load(void) {
    uint8_t cx, cy, i = 0;
    char c;
    goal_count = 0;
    hist_head = hist_len = 0;
    for (cy = 0; cy < BOARD_H; cy++) {
        for (cx = 0; cx < BOARD_W; cx++, i++) {
            c = map_char(cx << 1, cy << 1);
            switch (c) {
            case '#': case ' ': board[i] = CELL_WALL; break;
            case 'a': board[i] = CELL_GOAL; goal_count++; break;
            case 'B': board[i] = CELL_BOX; break;
            case 'W': board[i] = CELL_BOX | CELL_GOAL; goal_count++; break;
            case 'P': board[i] = 0; player_cell = i; break;
            default:  board[i] = 0; break;
            }
        }
    }
}

int16_t cell_px_x(uint8_t i) { return (int16_t)(i % BOARD_W) << 4; }
int16_t cell_px_y(uint8_t i) { return (int16_t)(i / BOARD_W) << 4; }

void board_draw_cell(uint8_t i) {
    uint16_t tx = (i % BOARD_W) << 1, ty = (i / BOARD_W) << 1;
    const char *c = cell_chars[(board[i] & (CELL_BOX | CELL_GOAL)) >> 1];
    map_set_tile(tx,     ty,     c[0]);
    map_set_tile(tx + 1, ty,     c[1]);
    map_set_tile(tx,     ty + 1, c[2]);
    map_set_tile(tx + 1, ty + 1, c[3]);
}

uint8_t board_try_move(uint8_t dir) {
    uint8_t t = player_cell + dir_off[dir];
    uint8_t b, pushed = 0;
    if (board[t] & CELL_WALL) return MOVE_BLOCKED;
    if (board[t] & CELL_BOX) {
        b = t + dir_off[dir];
        if (board[b] & (CELL_WALL | CELL_BOX)) return MOVE_BLOCKED;
        board[t] &= ~CELL_BOX;
        board[b] |= CELL_BOX;
        pushed = UNDO_PUSHED;
    }
    player_cell = t;
    hist[hist_head] = dir | pushed;
    hist_head = (hist_head + 1) & (HIST_MAX - 1);
    if (hist_len < HIST_MAX) hist_len++;
    return pushed ? MOVE_PUSH : MOVE_WALK;
}

uint8_t board_undo(void) {
    uint8_t v, dir, ahead;
    if (!hist_len) return UNDO_NONE;
    hist_head = (hist_head - 1) & (HIST_MAX - 1);
    hist_len--;
    v = hist[hist_head];
    dir = v & 3;
    if (v & UNDO_PUSHED) {                   /* pull the crate back to where the player stands */
        ahead = player_cell + dir_off[dir];
        board[ahead] &= ~CELL_BOX;
        board[player_cell] |= CELL_BOX;
        board_draw_cell(ahead);
        board_draw_cell(player_cell);
    }
    player_cell -= dir_off[dir];
    return UNDO_DONE | v;
}

uint8_t board_solved(void) {
    uint8_t i, n = 0;
    for (i = 0; i < BOARD_SIZE; i++)
        if ((board[i] & (CELL_BOX | CELL_GOAL)) == (CELL_BOX | CELL_GOAL)) n++;
    return n == goal_count;
}
