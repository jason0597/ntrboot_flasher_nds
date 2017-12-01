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

#include <cstdint>

#include "../include/ncgcpp/ntrcard.h"

using namespace ncgc;

namespace {
NTRCard card;

void platformIoDelay(std::uint32_t delay) {
    static_cast<void>(delay);
}

c::ncgc_err_t platformReset(c::ncgc_ncard_t *card) {
    static_cast<void>(card);
    card->state = c::NCGC_NRAW;
    return c::NCGC_EOK;
}

void platformSeedKey2(c::ncgc_ncard_t *card, std::uint64_t x, std::uint64_t y) {
    static_cast<void>(card); static_cast<void>(x); static_cast<void>(y);
}

c::ncgc_err_t platformSendCommand(c::ncgc_ncard_t *card, uint64_t cmd, uint32_t readsz, void *buf, uint32_t bufsz, c::ncgc_nflags_t flags) {
    static_cast<void>(card); static_cast<void>(cmd); static_cast<void>(readsz);
    static_cast<void>(buf); static_cast<void>(bufsz); static_cast<void>(flags);
    return c::NCGC_EOK;
}

c::ncgc_err_t platformSendWriteCommand(c::ncgc_ncard_t *card, uint64_t cmd, const void *buf, uint32_t bufsz, c::ncgc_nflags_t flags) {
    static_cast<void>(card); static_cast<void>(cmd);
    static_cast<void>(buf); static_cast<void>(bufsz); static_cast<void>(flags);
    return c::NCGC_EOK;
}

c::ncgc_err_t platformSpiTransact(c::ncgc_ncard_t *card, std::uint8_t bin, std::uint8_t *bout, bool last) {
    static_cast<void>(card); static_cast<void>(bin); static_cast<void>(bout);  static_cast<void>(last);
    return c::NCGC_EOK;
}

extern "C" int cxxtest() {
    c::ncgc_nplatform_t& p = card.rawState().platform;
    p.hw_key2 = true;
    p.io_delay = platformIoDelay;
    p.reset = platformReset;
    p.seed_key2 = platformSeedKey2;
    p.send_command = platformSendCommand;
    p.send_write_command = platformSendWriteCommand;
    p.spi_transact = platformSpiTransact;

    Err e;

    e = card.init();
    e = card.beginKey1();
    e = card.beginKey2();
    e = card.sendCommand(0xDEAD, nullptr, 0, NTRFlags().preDelay(0x910));

    return 0;
}
}
