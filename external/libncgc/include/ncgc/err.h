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

#ifndef NCGC_ERR_H
#define NCGC_ERR_H

typedef enum {
    NCGC_EOK = 0,
    NCGC_EUNSUP = 1,
    NCGC_EARG = 2,
    NCGC_ECSTATE = 10,
    NCGC_ECRESP = 11,
    NCGC_ECMISSING = 12,
    NCGC_EHERR = 20,
    NCGC_EUNK = 99
} ncgc_err_t;

const char *ncgc_err_desc(const ncgc_err_t err);

#endif // NCGC_ERR_H
