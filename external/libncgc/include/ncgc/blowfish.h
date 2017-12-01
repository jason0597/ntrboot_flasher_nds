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

#ifndef NCGC_BLOWFISH_H
#define NCGC_BLOWFISH_H

#include "nocpp.h"

#if !defined(__cplusplus)
#include <stdint.h>
#endif

#define NCGC_NBF_PS_N32 0x412

void ncgc_nbf_encrypt(const uint32_t ps[NCGC_NBF_PS_N32], uint32_t lr[2]);
void ncgc_nbf_decrypt(const uint32_t ps[NCGC_NBF_PS_N32], uint32_t lr[2]);
void ncgc_nbf_apply_key(uint32_t ps[NCGC_NBF_PS_N32], uint32_t key[3]);
#endif /* NCGC_BLOWFISH_H */