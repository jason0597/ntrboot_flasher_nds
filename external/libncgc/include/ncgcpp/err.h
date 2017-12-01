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

#ifndef NCGCPP_ERR_H
#define NCGCPP_ERR_H

#include "noc.h"

namespace ncgc {
namespace c {
using std::memcpy;
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::size_t;

#define NCGC_CPP_WRAPPER
extern "C" {
    #include "../ncgc/err.h"
}
#undef NCGC_CPP_WRAPPER
}
}

namespace ncgc {
class Err {
    c::ncgc_err_t err;
public:
    constexpr bool unsupported() const { return err == c::NCGC_EUNSUP; }
    constexpr int errNo() const { return static_cast<int>(err); }
    const char *desc() const { return c::ncgc_err_desc(err); }

    constexpr Err() : err(c::NCGC_EUNK) {}
    constexpr Err(const c::ncgc_err_t &from) : err(from) {}
    constexpr operator c::ncgc_err_t() const { return err; }
    constexpr operator bool() const { return err != c::NCGC_EOK; }
};
}
#endif // NCGCPP_ERR_H
