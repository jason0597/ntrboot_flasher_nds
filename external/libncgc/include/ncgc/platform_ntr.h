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

#ifndef NCGC_PLATFORM_NTR_H
#define NCGC_PLATFORM_NTR_H

#ifndef NCGC_NTRCARD_H
#error platform_ntr.h should not be manually included.
#endif

#include "nocpp.h"

void ncgc_platform_ntr_delay(uint32_t delay);
void ncgc_nplatform_ntr_init(ncgc_ncard_t *card, bool (*resetfn)(void));
#endif /* NCGC_PLATFORM_NTR_H */
