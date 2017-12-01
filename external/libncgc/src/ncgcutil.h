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

#ifndef NCGC_NCGCUTIL_H
#define NCGC_NCGCUTIL_H
// Utility macros.
// This file should only be included in _compiled files_, not headers!

#define BSWAP32(val) (((((val) >> 24) & 0xFF)) | ((((val) >> 16) & 0xFF) << 8) | ((((val) >> 8) & 0xFF) << 16) | (((val) & 0xFF) << 24))
#define BSWAP64(val) (((((val) >> 56) & 0xFF)) | ((((val) >> 48) & 0xFF) << 8) | ((((val) >> 40) & 0xFF) << 16) | ((((val) >> 32) & 0xFF) << 24) | \
    ((((val) >> 24) & 0xFF) << 32) | ((((val) >> 16) & 0xFF) << 40) | ((((val) >> 8) & 0xFF) << 48) | ((((val)) & 0xFF) << 56))
#define NMIN(v1, v2) ((v1) < (v2) ? (v1) : (v2))
#define NMAX(v1, v2) ((v1) > (v2) ? (v1) : (v2))
#endif
