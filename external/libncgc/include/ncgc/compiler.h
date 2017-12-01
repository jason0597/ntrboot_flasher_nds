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

#ifndef NCGC_COMPILER_H
#define NCGC_COMPILER_H

#ifdef __GNUC__
    #define __ncgc_must_check __attribute__((warn_unused_result))
    #define __ncgc_static_assert(x, y) _Static_assert(x, y)
#else
    #define __ncgc_must_check
    #define __ncgc_static_assert(x, y)
#endif
#endif /* NCGC_COMPILER_H */
