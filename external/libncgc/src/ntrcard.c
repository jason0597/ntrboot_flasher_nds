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

#include <string.h>
#include <stdint.h>

#include "../include/ncgc/err.h"
#include "../include/ncgc/compiler.h"
#include "../include/ncgc/ntrcard.h"
#include "../include/ncgc/blowfish.h"

#include "ncgcutil.h"

#define FLAGS_WR               (1u << 30)
#define FLAGS_DELAY_PULSE_CLK  (1u << 28)              // Pulse clock during pre- and post(?)-delays
#define FLAGS_CLK_SLOW         (1u << 27)              // Transfer clock rate (0 = 6.7MHz, 1 = 4.2MHz)
#define FLAGS_SEC_CMD          (1u << 22)              // The command transfer will be hardware encrypted (KEY2)
#define FLAGS_DELAY2(n)        (((n) & 0x3Fu) << 16)   // Transfer delay length part 2
#define FLAGS_DELAY2_MASK      (FLAGS_DELAY2(0x3F))
#define FLAGS_SEC_EN           (1u << 14)              // Security enable
#define FLAGS_SEC_DAT          (1u << 13)              // The data transfer will be hardware encrypted (KEY2)
#define FLAGS_DELAY1(n)        ((n) & 0x1FFFu)         // Transfer delay length part 1
#define FLAGS_DELAY1_MASK      (FLAGS_DELAY1(0x1FFF))

#define CMD_RAW_DUMMY           0x9Fu
#define CMD_RAW_CHIPID          0x90u
#define CMD_RAW_HEADER_READ     0x00u
#define CMD_RAW_ACTIVATE_KEY1   0x3Cu

#define CMD_KEY1_INIT_KEY2      0x4u
#define CMD_KEY1_CHIPID         0x1u
#define CMD_KEY1_SECURE_READ    0x2u
#define CMD_KEY1_ACTIVATE_KEY2  0xAu

#define CMD_KEY2_DATA_READ      0xB7u
#define CMD_KEY2_CHIPID         0xB8u

#define P(card)                 ((card)->platform)
#define F(flags)                ((ncgc_nflags_t) { (flags) })

static uint64_t key1_construct(ncgc_ncard_t *const card, const uint8_t cmdarg, const uint16_t arg, const uint32_t ij) {
    // C = cmd, A = arg
    // KK KK JK JJ II AI AA CA
    const uint32_t k = card->key1.k++;
    uint64_t cmd = ((cmdarg & 0xF) << 4) |
        ((arg & 0xF000ull) >> 12 /* << 0 - 12 */) | ((arg & 0xFF0ull) << 4 /* 8 - 4 */) | ((arg & 0xFull) << 20 /* 20 - 0 */) |
        ((ij & 0xF00000ull) >> 4 /* << 16 - 20 */) | ((ij & 0xFF000ull) << 12 /* 24 - 12 */) | ((ij & 0xFF0ull) << 28 /* 32 - 4 */) |
        ((ij & 0xFull) << 44 /* 44 - 0 */) | ((k & 0xF0000ull) << 24 /* 40 - 16 */) | ((k & 0xFF00ull) << 40 /* 48 - 8 */) |
        ((k & 0xFFull) << 56 /* 56 - 0 */);
    cmd = BSWAP64(cmd);
    ncgc_nbf_encrypt(card->key1.ps, (void *) &cmd);
    return BSWAP64(cmd);
}

static __ncgc_must_check ncgc_err_t key1_cmd(ncgc_ncard_t *const card, const uint8_t cmdarg, const uint16_t arg, const uint32_t ij,
    const uint32_t read_size, void *const dest, const uint32_t flags) {
    uint64_t cmd = key1_construct(card, cmdarg, arg, ij);
    return P(card).send_command(card, cmd, read_size, dest, read_size, F(flags));
}

static void seed_key2(ncgc_ncard_t *const card) {
    const uint8_t seed_bytes[8] = {0xE8, 0x4D, 0x5A, 0xB1, 0x17, 0x8F, 0x99, 0xD5};
    card->key2.x = seed_bytes[card->key2.seed_byte & 7] + (((uint64_t)(card->key2.mn)) << 15) + 0x6000;
    card->key2.y = 0x5C879B9B05ull;

    if (P(card).hw_key2) {
        P(card).seed_key2(card, card->key2.x, card->key2.y);
    }
}

static __ncgc_must_check ncgc_err_t init_common(ncgc_ncard_t *const card) {
    ncgc_err_t r;

    if (card->state != NCGC_NPREINIT) {
        if ((r = P(card).reset(card))) {
            return r;
        }
    }

    if (card->state != NCGC_NPREINIT) {
        return NCGC_ECSTATE;
    }

    if ((r = P(card).send_command(card, CMD_RAW_DUMMY, 0x2000, NULL, 0, F(FLAGS_CLK_SLOW | FLAGS_DELAY2(0x18))))) {
        return r;
    }

    P(card).io_delay(0x40000); // is this too much?
    return NCGC_EOK;
}

static __ncgc_must_check ncgc_err_t init_chipid(ncgc_ncard_t *const card) {
    ncgc_err_t r;
    if ((r = P(card).send_command(card, CMD_RAW_CHIPID, 4, &card->raw_chipid, 4, F(FLAGS_CLK_SLOW | FLAGS_DELAY1(0x18))))) {
        return r;
    }
    return NCGC_EOK;
}

static __ncgc_must_check ncgc_err_t init_header(ncgc_ncard_t *const card, void *const buf) {
    char ourbuf[0x68] = {0};
    char *usedbuf = buf ? buf : ourbuf;

    ncgc_err_t r = P(card).send_command(card,
        CMD_RAW_HEADER_READ, 0x1000, usedbuf, buf ? 0x1000 : sizeof(ourbuf),
        F(FLAGS_CLK_SLOW | FLAGS_DELAY1(0x657) | FLAGS_DELAY2(0x01)));
    if (r) {
        return r;
    }

    card->hdr.game_code = *(uint32_t *)(usedbuf + 0xC);
    card->hdr.key1_romcnt = card->key1.romcnt = *(uint32_t *)(usedbuf + 0x64);
    card->hdr.key2_romcnt = card->key2.romcnt = *(uint32_t *)(usedbuf + 0x60);
    card->key2.seed_byte = *(uint8_t *)(usedbuf + 0x13);
    return NCGC_EOK;
}

ncgc_err_t ncgc_ninit_order(ncgc_ncard_t *const card, void *const buf, bool header_first) {
    ncgc_err_t r;
    if (
        (r = init_common(card)) ||
        (header_first && (r = init_header(card, buf))) ||
        (r = init_chipid(card)) ||
        (!header_first && (r = init_header(card, buf)))
       ) {
        return r;
    }

    card->state = NCGC_NRAW;
    return NCGC_EOK;
}

void ncgc_nsetup_blowfish(ncgc_ncard_t *const card, const uint8_t ps[NCGC_NBF_PS_N32*4]) {
    card->key1.key[0] = card->hdr.game_code;
    card->key1.key[1] = card->hdr.game_code >> 1;
    card->key1.key[2] = card->hdr.game_code << 1;
    memcpy(card->key1.ps, ps, sizeof(card->key1.ps));
    __ncgc_static_assert(sizeof(card->key1.ps) == 0x1048, "Wrong Blowfish PS size");

    ncgc_nbf_apply_key(card->key1.ps, card->key1.key);
    ncgc_nbf_apply_key(card->key1.ps, card->key1.key);
}

ncgc_err_t ncgc_nbegin_key1(ncgc_ncard_t *const card) {
    if (card->state != NCGC_NRAW) {
        return NCGC_ECSTATE;
    }

    ncgc_err_t r;
    card->key2.mn = 0xC99ACE;
    card->key1.ij = 0x11A473;
    card->key1.k = 0x39D46;
    card->key1.l = 0;

    // 00 KK KK 0K JJ IJ II 3C
    if ((r = P(card).send_command(card,
        CMD_RAW_ACTIVATE_KEY1 |
            ((card->key1.ij & 0xFF0000ull) >> 8) | ((card->key1.ij & 0xFF00ull) << 8) | ((card->key1.ij & 0xFFull) << 24) |
            ((card->key1.k & 0xF0000ull) << 16) | ((card->key1.k & 0xFF00ull) << 32) | ((card->key1.k & 0xFFull) << 48),
        0, NULL, 0, F(card->key2.romcnt & (FLAGS_CLK_SLOW | FLAGS_DELAY2_MASK | FLAGS_DELAY1_MASK))))) {
        return r;
    }

    card->key1.romcnt = (card->key2.romcnt & (FLAGS_WR | FLAGS_CLK_SLOW)) |
        ((card->hdr.key1_romcnt & (FLAGS_CLK_SLOW | FLAGS_DELAY1_MASK)) +
        ((card->hdr.key1_romcnt & FLAGS_DELAY2_MASK) >> 16)) | FLAGS_DELAY_PULSE_CLK;
    if ((r = key1_cmd(card, CMD_KEY1_INIT_KEY2, card->key1.l, card->key2.mn, 0, NULL, card->key1.romcnt))) {
        return r;
    }

    seed_key2(card);
    card->key1.romcnt |= FLAGS_SEC_EN | FLAGS_SEC_DAT;

    if ((r = key1_cmd(card, CMD_KEY1_CHIPID, card->key1.l, card->key1.ij, 4, &card->key1.chipid, card->key1.romcnt))) {
        return r;
    }
    if (card->raw_chipid != card->key1.chipid) {
        card->state = NCGC_NUNKNOWN;
        return NCGC_ECRESP;
    }
    card->state = NCGC_NKEY1;
    return NCGC_EOK;
}

ncgc_err_t ncgc_nread_secure_area(ncgc_ncard_t *const card, void *const dest) {
    ncgc_err_t r;
    if (card->state != NCGC_NKEY1) {
        return NCGC_ECSTATE;
    }

    uint32_t secure_area_romcnt = (card->hdr.key1_romcnt & (FLAGS_CLK_SLOW | FLAGS_DELAY1_MASK | FLAGS_DELAY2_MASK)) |
        FLAGS_SEC_EN | FLAGS_SEC_DAT | FLAGS_DELAY_PULSE_CLK;
    char *const dest8 = dest;
    for (uint16_t c = 4; c < 8; ++c) {
        // TODO handle chipid high-bit set
        if ((r = key1_cmd(card,
            CMD_KEY1_SECURE_READ, c, card->key1.ij, 0x1000,
            dest8 + (c - 4) * 0x1000, secure_area_romcnt))) {
            return r;
        }
    }

    return NCGC_EOK;
}

ncgc_err_t ncgc_nbegin_key2(ncgc_ncard_t *const card) {
    if (card->state != NCGC_NKEY1) {
        return NCGC_ECSTATE;
    }

    ncgc_err_t r;
    if ((r = key1_cmd(card, CMD_KEY1_ACTIVATE_KEY2, card->key1.l, card->key1.ij, 0, NULL, card->key1.romcnt))) {
        return r;
    }
    card->key2.romcnt = card->hdr.key2_romcnt & (FLAGS_CLK_SLOW | FLAGS_SEC_CMD | FLAGS_DELAY2_MASK | FLAGS_SEC_EN | FLAGS_SEC_DAT | FLAGS_DELAY1_MASK);

    if ((r = P(card).send_command(card,
        CMD_KEY2_CHIPID, 4, &card->key2.chipid, 4, F(card->key2.romcnt)))) {
        return r;
    }
    if (card->key2.chipid != card->raw_chipid) {
        card->state = NCGC_NUNKNOWN;
        return NCGC_ECRESP;
    }
    card->state = NCGC_NKEY2;
    return NCGC_EOK;
}

static __ncgc_must_check ncgc_err_t key2_read(ncgc_ncard_t *const card, uint32_t address, char *buf) {
    return P(card).send_command(card, CMD_KEY2_DATA_READ | (((uint64_t) BSWAP32(address)) << 8), 0x200, buf, buf ? 0x200 : 0, F(card->key2.romcnt));
}

ncgc_err_t ncgc_nread_data(ncgc_ncard_t *const card, const uint32_t address, void *const buf, const size_t size) {
    if (card->state != NCGC_NKEY2) {
        return NCGC_ECSTATE;
    }

    size_t size_left = size;
    uint32_t cur_addr = (address & ~0x1FF);
    char *cur_buf = buf;
    ncgc_err_t r;

    if (cur_addr < address) {
        size_t bytes_completed = NMIN(size, 0x200 - (address - cur_addr));
        char buf[0x200];
        if ((r = key2_read(card, cur_addr, buf))) {
            return r;
        }

        if (cur_buf) {
            memcpy(cur_buf, buf + (address - cur_addr), bytes_completed);
            cur_buf += bytes_completed;
        }
        size_left -= bytes_completed;
        cur_addr += 0x200;
    }

    while (size_left >= 0x200) {
        if ((r = key2_read(card, cur_addr, cur_buf))) {
            return r;
        }
        size_left -= 0x200;
        if (cur_buf) {
            cur_buf += 0x200;
        }
        cur_addr += 0x200;
    }

    if (size_left > 0) {
        char buf[0x200];
        if ((r = key2_read(card, cur_addr, buf))) {
            return r;
        }
        if (cur_buf) {
            memcpy(cur_buf, buf, size_left);
        }
    }

    return NCGC_EOK;
}

ncgc_err_t __ncgc_must_check ncgc_nspi_command(ncgc_ncard_t *const card, const uint8_t *const command, const size_t command_length,
                                            uint8_t *const response, const size_t response_length) {
    size_t ctr;
    ncgc_err_t r;
    for (ctr = 0; ctr < command_length; ++ctr) {
        if ((r = P(card).spi_transact(card, command[ctr], NULL, !response_length && (ctr + 1) == command_length))) {
            return r;
        }
    }
    for (ctr = 0; ctr < response_length; ++ctr) {
        if ((r = P(card).spi_transact(card, 0xFF, response ? response + ctr : NULL, (ctr + 1) == response_length))) {
            return r;
        }
    }
    return NCGC_EOK;
}
