/* libncgc
 * Copyright (C) 2017 angelsl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __MINGW32__
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "../include/ncgc/ntrcard.h"
#include "ncgcutil.h"

extern const char _binary_ntr_blowfish_bin_start;

enum op_type {
    COMMAND, DELAY, RESET, SEED_KEY2
};

const char *op_type_str(enum op_type op_type) {
    switch (op_type) {
        case COMMAND:
            return "COMMAND";
        case DELAY:
            return "DELAY";
        case RESET:
            return "RESET";
        case SEED_KEY2:
            return "SEED_KEY2";
        default:
            return "UNKNOWN";
    }
}

struct op {
    size_t index;
    enum op_type op_type;
    union {
        struct {
            uint64_t cmd;
            uint32_t size;
            uint32_t flags;
        } command;
        struct {
            uint64_t x;
            uint64_t y;
        } seed;
        uint32_t delay;
    };
};

size_t cur_op = 0;
struct op ops[] = {
    { /* cur_op = 0 */
        .op_type = RESET
    }, { /* cur_op = 1 */
        .op_type = COMMAND,
        .command = { .cmd = 0x9F00000000000000ull, .size = 0x2000, .flags = 0x8180000 }
    }, { /* cur_op = 2 */
        .op_type = DELAY,
        .delay = 0x40000
    }, { /* cur_op = 3 */
        .op_type = COMMAND,
        .command = { .cmd = 0x9000000000000000ull, .size = 0x4, .flags = 0x8000018 }
    }, { /* cur_op = 4 */
        .op_type = COMMAND,
        .command = { .cmd = 0x0000000000000000ull, .size = 0x1000, .flags = 0x8010657 }
    }, { /* cur_op = 5 */
        .op_type = COMMAND,
        .command = { .cmd = 0x3C11A473039D4600ull, .size = 0x0, .flags = 0x10657 }
    }, { /* cur_op = 6 */
        .op_type = COMMAND,
        .command = { .cmd = 0x07F76F3B77AF949Full, .size = 0x0, .flags = 0x18000910 }
    }, { /* cur_op = 7 */
        .op_type = SEED_KEY2,
        .seed = { .x = 0x64CD6760E8, .y = 0x5C879B9B05 }
    }, { /* cur_op = 8 */
        .op_type = COMMAND,
        .command = { .cmd = 0xF66CE157F12C1FFEull, .size = 0x4, .flags = 0x18006910 }
    }, { /* cur_op = 9 */
        .op_type = COMMAND,
        .command = { .cmd = 0xB322615F1A1E68A5ull, .size = 0x0, .flags = 0x18006910 }
    }, { /* cur_op = 10 */
        .op_type = COMMAND,
        .command = { .cmd = 0xB800000000000000ull, .size = 0x4, .flags = 0x416657 }
    }, { /* cur_op = 11 */
        .op_type = COMMAND,
        .command = { .cmd = 0xB700000000000000ull, .size = 0x200, .flags = 0x416657 }
    }, { /* cur_op = 12 */
        .op_type = COMMAND,
        .command = { .cmd = 0xB700000200000000ull, .size = 0x200, .flags = 0x416657 }
    }, { /* cur_op = 13 */
        .op_type = COMMAND,
        .command = { .cmd = 0xB700000400000000ull, .size = 0x200, .flags = 0x416657 }
    }, { /* cur_op = 14 */
        .op_type = COMMAND,
        .command = { .cmd = 0xB700000600000000ull, .size = 0x200, .flags = 0x416657 }
    }
};
const size_t n_ops = sizeof(ops)/sizeof(struct op);

bool failed = false;

static void mmemcpy(void *dest, const void* src, size_t dest_sz, size_t count_sz, size_t offs) {
    if (!dest_sz || !count_sz || offs > dest_sz) {
        return;
    }
    dest_sz -= offs;
    memcpy((char *)dest + offs, src, dest_sz < count_sz ? dest_sz : count_sz);
}

static void write_response(size_t op_no, void *const dest, const uint32_t dest_size) {
    switch (op_no) {
        default:
            failed = true;
            fprintf(stderr, "FAIL: COMMAND (%zu) no response\n", op_no);
            break;
        case 1:
        case 11:
        case 12:
        case 13:
        case 14:
            break;
        case 3:
        case 8:
        case 10: {
            mmemcpy(dest, "\xC2\x07\0\0", dest_size, 4, 0);
            break;
        }
        case 4: {
            mmemcpy(dest, "ABXK", dest_size, 4, 0xC);
            mmemcpy(dest, "\0", dest_size, 1, 0x13);
            mmemcpy(dest, "\x57\x66\x41\0", dest_size, 4, 0x60);
            mmemcpy(dest, "\xf8\x08\x18\x08", dest_size, 4, 0x64);
            break;
        }
    }
}

static struct op *next_op(enum op_type op_type) {
    if (cur_op < n_ops) {
        ops[cur_op].index = cur_op;
        struct op* op = ops + cur_op++;
        if (op->op_type != op_type) {
            failed = true;
            fprintf(stderr, "FAIL: %s (%zu) expected, %s actually\n", op_type_str(op_type), op->index, op_type_str(op->op_type));
            return NULL;
        }
        return op;
    } else {
        failed = true;
        fprintf(stderr, "FAIL: ran out of ops\n");
        return NULL;
    }
}

static ncgc_err_t send_command(ncgc_ncard_t *const card, const uint64_t cmdle, const uint32_t read_size,
        void *const dest, const uint32_t dest_size, const ncgc_nflags_t flags) {
    (void)card; (void)dest; (void)dest_size;
    const uint64_t cmd = BSWAP64(cmdle);
    #ifdef PRINT
    printf(
        "{ /* cur_op = %zu */\n"
        "    .op_type = COMMAND,\n"
        "    .command = { .cmd = 0x%" PRIX64 ", .size = 0x%" PRIX32 ", .flags = 0x%" PRIX32 " }\n"
        "}, ",
        cur_op, cmd, read_size, flags.flags
    );
    #endif

    struct op *op = next_op(COMMAND);
    if (op) {
        if (op->command.cmd != cmd) {
            failed = true;
            fprintf(stderr, "FAIL: COMMAND (%zu) expected cmd 0x%" PRIX64 ", actual cmd 0x%" PRIX64 "\n", op->index, op->command.cmd, cmd);
        } else if (read_size) {
            write_response(op->index, dest, dest_size);
        }

        if (op->command.size != read_size) {
            failed = true;
            fprintf(stderr, "FAIL: COMMAND (%zu) expected size 0x%" PRIX32 ", actual size 0x%" PRIX32 "\n", op->index, op->command.size, read_size);
        }

        if (op->command.flags != flags.flags) {
            failed = true;
            fprintf(stderr, "FAIL: COMMAND (%zu) expected flags 0x%" PRIX32 ", actual flags 0x%" PRIX32 "\n", op->index, op->command.flags, flags.flags);
        }
    }
    return NCGC_EOK;
}

static ncgc_err_t send_write_command(ncgc_ncard_t *const card, const uint64_t cmdle,
        const void *const dest, const uint32_t dest_size, const ncgc_nflags_t flags) {
    (void)card; (void)cmdle; (void)dest; (void)dest_size; (void)flags;
    return NCGC_EOK;
}

static ncgc_err_t spi_transact(ncgc_ncard_t *const card, uint8_t in, uint8_t *out, bool last) {
    (void)card; (void)in; (void)out; (void)last;
    return NCGC_EOK;
}

static void io_delay(uint32_t delay) {
    #ifdef PRINT
    printf(
        "{ /* cur_op = %zu */\n"
        "    .op_type = DELAY,\n"
        "    .delay = 0x%" PRIX32 "\n"
        "}, ",
        cur_op, delay
    );
    #endif

    struct op *op = next_op(DELAY);
    if (op) {
        if (op->delay != delay) {
            failed = true;
            fprintf(stderr, "FAIL: DELAY (%zu) expected delay 0x%" PRIX32 ", actual delay 0x%" PRIX32 "\n", op->index, op->delay, delay);
        }
    }
}

static void seed_key2(ncgc_ncard_t *const card, uint64_t x, uint64_t y) {
    (void)card;
    #ifdef PRINT
    printf(
        "{ /* cur_op = %zu */\n"
        "    .op_type = SEED_KEY2,\n"
        "    .seed = { .x = 0x%" PRIX64 ", .y = 0x%" PRIX64 " }\n"
        "}, ",
        cur_op, x, y
    );
    #endif

    struct op *op = next_op(SEED_KEY2);
    if (op) {
        if (op->seed.x != x) {
            failed = true;
            fprintf(stderr, "FAIL: SEED_KEY2 (%zu) expected x 0x%" PRIX64 ", actual x 0x%" PRIX64 "\n", op->index, op->seed.x, x);
        }

        if (op->seed.y != y) {
            failed = true;
            fprintf(stderr, "FAIL: SEED_KEY2 (%zu) expected y 0x%" PRIX64 ", actual y 0x%" PRIX64 "\n", op->index, op->seed.y, y);
        }
    }
}

static ncgc_err_t reset(ncgc_ncard_t *const card) {
    #ifdef PRINT
    printf(
        "{ /* cur_op = %zu */\n"
        "    .op_type = RESET\n"
        "}, ",
        cur_op
    );
    #endif

    card->state = NCGC_NPREINIT;
    next_op(RESET);
    return NCGC_EOK;
}

static ncgc_ncard_t card = {
    .platform = {
        .data = { .int_data = 0 },
        .reset = reset,
        .send_command = send_command,
        .send_write_command = send_write_command,
        .spi_transact = spi_transact,
        .io_delay = io_delay,
        .seed_key2 = seed_key2,
        .hw_key2 = true
    }
};

extern int cxxtest(void);

int main() {
    ncgc_err_t r;
    if ((r = ncgc_ninit(&card, NULL))) {
        fprintf(stderr, "FAIL: ncgc_ninit = %d (%s)\n", r, ncgc_err_desc(r));
        failed = true;
    }

    ncgc_nsetup_blowfish(&card, (void *) &_binary_ntr_blowfish_bin_start);

    if ((r = ncgc_nbegin_key1(&card))) {
        fprintf(stderr, "FAIL: ncgc_nbegin_key1 = %d (%s)\n", r, ncgc_err_desc(r));
        failed = true;
    }

    if ((r = ncgc_nbegin_key2(&card))) {
        fprintf(stderr, "FAIL: ncgc_nbegin_key2 = %d (%s)\n", r, ncgc_err_desc(r));
        failed = true;
    }

    char buf[1642];
    if ((r = ncgc_nread_data(&card, 117, buf, sizeof(buf)))) {
        fprintf(stderr, "FAIL: ncgc_nread_data = %d (%s)\n", r, ncgc_err_desc(r));
        failed = true;
    }
    #ifdef PRINT
    puts("");
    #endif
    if (failed) {
        fprintf(stderr, "Failed.\n");
    } else {
        fprintf(stderr, "Success.\n");
    }

    fprintf(stderr, "Running C++ test.\n");
    failed |= !!cxxtest();

    return failed;
}
