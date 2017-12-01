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

#include "../include/ncgc/blowfish.h"

#include <stdint.h>

#include "ncgcutil.h"

void ncgc_nbf_encrypt(const uint32_t ps[NCGC_NBF_PS_N32], uint32_t lr[2]) {
    uint32_t x = lr[1];
    uint32_t y = lr[0];
    uint32_t z;

    for(int i = 0; i < 0x10; ++i) {
        z = ps[i] ^ x;
        x = ps[0x012 + ((z >> 24) & 0xFF)];
        x = ps[0x112 + ((z >> 16) & 0xFF)] + x;
        x = ps[0x212 + ((z >> 8) & 0xFF)] ^ x;
        x = ps[0x312 + ((z >> 0) & 0xFF)] + x;
        x = y ^ x;
        y = z;
    }

    lr[0] = x ^ ps[0x10];
    lr[1] = y ^ ps[0x11];
}

void ncgc_nbf_decrypt(const uint32_t ps[NCGC_NBF_PS_N32], uint32_t lr[2]) {
    uint32_t x = lr[1];
    uint32_t y = lr[0];
    uint32_t z;

    for(int i = 0x11; i > 1; --i) {
        z = ps[i] ^ x;
        x = ps[0x012 + ((z >> 24) & 0xFF)];
        x = ps[0x112 + ((z >> 16) & 0xFF)] + x;
        x = ps[0x212 + ((z >> 8) & 0xFF)] ^ x;
        x = ps[0x312 + ((z >> 0) & 0xFF)] + x;
        x = y ^ x;
        y = z;
    }

    lr[0] = x ^ ps[1];
    lr[1] = y ^ ps[0];
}

#define NCGC_NBF_P_N32 0x12
void ncgc_nbf_apply_key(uint32_t ps[NCGC_NBF_PS_N32], uint32_t key[3]) {
    uint32_t scratch[2] = {0};

    ncgc_nbf_encrypt(ps, key + 1);
    ncgc_nbf_encrypt(ps, key);

    for(int i = 0; i < NCGC_NBF_P_N32; ++i) {
        ps[i] = ps[i] ^ BSWAP32(key[i%2]);
    }

    for(int i = 0; i < NCGC_NBF_PS_N32; i += 2) {
        ncgc_nbf_encrypt(ps, scratch);
        ps[i] = scratch[1];
        ps[i + 1] = scratch[0];
    }
}