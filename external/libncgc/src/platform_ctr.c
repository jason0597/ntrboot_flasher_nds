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

#include <stdbool.h>
#include <stdint.h>

#include "../include/ncgc/ntrcard.h"

#define REG_MCNT                (*(volatile uint16_t *)0x10164000)
#define REG_MDATA               (*(volatile uint8_t *)0x10164002)
#define REG_ROMCNT              (*(volatile uint32_t *)0x10164004)
#define REG_CMDP                ((volatile uint8_t *)0x10164008)
#define REG_CMD                 (*(volatile uint64_t *)0x10164008)
#define REG_SEEDX_L             (*(volatile uint32_t *)0x10164010)
#define REG_SEEDY_L             (*(volatile uint32_t *)0x10164014)
#define REG_SEEDX_H             (*(volatile uint16_t *)0x10164018)
#define REG_SEEDY_H             (*(volatile uint16_t *)0x1016401A)
#define REG_FIFO                (*(volatile uint32_t *)0x1016401C)

#include "platform_ntrcommon.c"

#define REG_CARDCONF            (*(volatile uint16_t *)0x1000000C)
#define REG_CARDCONF2           (*(volatile uint8_t *)0x10000010)
#define REG_CTRCARD_SECCNT      (*(volatile uint32_t *)0x10004008)

void ncgc_nplatform_ctr_wait_for_card(void) {
    while (REG_CARDCONF2 & 0x1);
}

bool ncgc_nplatform_ctr_card_inserted(void) {
    return !(REG_CARDCONF2 & 0x1);
}

void ncgc_platform_ctr_delay(uint32_t delay) {
    io_delay(delay);
}

static ncgc_err_t reset(ncgc_ncard_t *const card) {
    if (REG_CARDCONF2 & 0x1) {
        return NCGC_ECMISSING;
    }

    REG_CARDCONF2 = 0x0C;
    REG_CARDCONF &= ~3;
    if (REG_CARDCONF2 == 0xC) {
        while (REG_CARDCONF2 != 0);
    }
    if (REG_CARDCONF2 != 0) {
        return NCGC_EHERR;
    }
    REG_CARDCONF2 = 0x4;
    while (REG_CARDCONF2 != 0x4);
    REG_CARDCONF2 = 0x8;
    while (REG_CARDCONF2 != 0x8);

    REG_CTRCARD_SECCNT &= 0xFFFFFFFB;
    io_delay(0x40000);

    REG_ROMCNT = ROMCNT_NRESET;
    REG_CARDCONF &= ~3;
    REG_CARDCONF &= ~0x100;
    REG_MCNT = MCNT_CR1_ENABLE;
    io_delay(0x40000);

    REG_ROMCNT = 0;
    REG_MCNT &= 0xFF;
    io_delay(0x40000);

    REG_MCNT |= (MCNT_CR1_ENABLE | MCNT_CR1_IRQ);
    REG_ROMCNT = ROMCNT_NRESET | ROMCNT_SEC_SEED;
    while (REG_ROMCNT & ROMCNT_BUSY);

    card->state = NCGC_NPREINIT;
    return NCGC_EOK;
}

void ncgc_nplatform_ctr_init(ncgc_ncard_t *card) {
    card->platform = (ncgc_nplatform_t) {
        .data = { .int_data = 0 },
        .reset = reset,
        .send_command = send_command,
        .send_write_command = send_write_command,
        .spi_transact = spi_transact,
        .io_delay = ncgc_platform_ctr_delay,
        .seed_key2 = seed_key2,
        .hw_key2 = true
    };
}
