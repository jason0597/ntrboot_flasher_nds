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

#ifndef NCGCPP_NTRCARD_H
#define NCGCPP_NTRCARD_H

#include "noc.h"

#include <cstdint>
#include <cstddef>
#include <cstring>

#include "err.h"

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
    #include "../ncgc/ntrcard.h"
}
#undef NCGC_CPP_WRAPPER

constexpr static std::uint64_t cmdBytesToQword(const uint8_t (&command)[8]) {
    return
        ((std::uint64_t(0) | command[0])      ) |
        ((std::uint64_t(0) | command[1]) <<  8) |
        ((std::uint64_t(0) | command[2]) << 16) |
        ((std::uint64_t(0) | command[3]) << 24) |
        ((std::uint64_t(0) | command[4]) << 32) |
        ((std::uint64_t(0) | command[5]) << 40) |
        ((std::uint64_t(0) | command[6]) << 48) |
        ((std::uint64_t(0) | command[7]) << 56);
}
static_assert(cmdBytesToQword({1, 2, 3, 4, 5, 6, 7, 8}) == 0x807060504030201, "cmdBytesToQword is wrong");
}
}

namespace ncgc {
inline void delay(std::uint32_t delay) {
#if defined(NCGC_PLATFORM_NTR)
    c::ncgc_platform_ntr_delay(delay);
#elif defined(NCGC_PLATFORM_CTR)
    c::ncgc_platform_ctr_delay(delay);
#else
    static_cast<void>(delay);
#endif
}

enum class NTRState {
    Unknown = 0,
    Preinit = 10,
    Raw = 11,
    Key1 = 12,
    Key2 = 13
};

class NTRCard;
class NTRFlags {
    /// The raw ROMCNT value.
    std::uint32_t romcnt;
    friend NTRCard;
public:
    constexpr bool bit(uint32_t bit) const { return !!(romcnt & (1 << bit)); }
    constexpr NTRFlags bit(uint32_t bit, bool set) const { return (romcnt & ~(1 << bit)) | (set ? (1 << bit) : 0); }

    /// Returns the delay before the response to a KEY1 command (KEY1 gap1)
    constexpr std::uint16_t preDelay() const { return static_cast<uint16_t>(romcnt & 0x1FFF); }
    /// Returns the delay after the response to a KEY1 command (KEY1 gap2)
    constexpr std::uint16_t postDelay() const { return static_cast<uint16_t>((romcnt >> 16) & 0x3F); }
    /// Returns true if clock pulses should be sent, and the KEY2 state advanced, during the pre- and post(?)-delays
    constexpr bool delayPulseClock() const { return bit(28); }
    /// Returns true if the command is KEY2-encrypted
    constexpr bool key2Command() const { return bit(22) && bit(14); }
    /// Returns true if the command is KEY2-encrypted
    constexpr bool key2Response() const { return bit(13) && bit(14); }
    /// Returns true if the slower CLK rate should be used (usually for raw commands)
    constexpr bool slowClock() const { return bit(27); }

    /// Sets the the delay before the response to a KEY1 command (KEY1 gap1)
    constexpr NTRFlags preDelay(std::uint16_t value) const { return (romcnt & ~0x1FFF) | (value & 0x1FFF); }
    /// Sets the delay after the response to a KEY1 command (KEY1 gap2)
    constexpr NTRFlags postDelay(std::uint16_t value) const { return (romcnt & ~(0x3F << 16)) | ((value & 0x3F) << 16); }
    /// Set if clock pulses should be sent, and the KEY2 state advanced, during the pre- and post(?)-delays
    constexpr NTRFlags delayPulseClock(bool value) const { return bit(28, value); }
    /// Set if the command is KEY2-encrypted
    constexpr NTRFlags key2Command(bool value) const { return bit(22, value).bit(14, value || bit(13)); }
    /// Set if the command is KEY2-encrypted
    constexpr NTRFlags key2Response(bool value) const { return bit(13, value).bit(14, value || bit(22)); }
    /// Set if the slower CLK rate should be used (usually for raw commands)
    constexpr NTRFlags slowClock(bool value) const { return bit(27, value); }

    constexpr operator std::uint32_t() const { return romcnt; }
    constexpr NTRFlags(const std::uint32_t& from) : romcnt(from) {}
    constexpr NTRFlags() : romcnt(0) {}
};

class NTRCard {
public:
#if defined(NCGC_PLATFORM_NTR)
    template<typename ResetFn>
    inline NTRCard(ResetFn f) {
        c::ncgc_nplatform_ntr_init(&_card, f);
    }
#elif defined(NCGC_PLATFORM_CTR)
    inline NTRCard() {
        c::ncgc_nplatform_ctr_init(&_card);
    }

    inline static void waitForCard() {
        c::ncgc_nplatform_ctr_wait_for_card();
    }

    inline static bool cardInserted() {
        return c::ncgc_nplatform_ctr_card_inserted();
    }
#elif defined(NCGC_PLATFORM_TEST)
    inline NTRCard() {}
#endif

    // you cannot copy this class, it does not make sense
    NTRCard(const NTRCard& other) = delete;
    NTRCard& operator=(const NTRCard& other) = delete;

    inline __ncgc_must_check Err init(void *buffer = nullptr, bool header_first = false) {
        return c::ncgc_ninit_order(&_card, buffer, header_first);
    }

    inline __ncgc_must_check Err beginKey1() {
        return c::ncgc_nbegin_key1(&_card);
    }

    inline __ncgc_must_check Err beginKey2() {
        return c::ncgc_nbegin_key2(&_card);
    }

    inline void setBlowfishState(const std::uint8_t (&ps)[NCGC_NBF_PS_N32*4], bool as_is = false) {
        if (as_is) {
            c::ncgc_nsetup_blowfish_as_is(&_card, ps);
        } else {
            c::ncgc_nsetup_blowfish(&_card, ps);
        }
    }

    inline __ncgc_must_check Err readData(const std::uint32_t address, void *const buf, const std::size_t size) {
        return c::ncgc_nread_data(&_card, address, buf, size);
    }

    inline __ncgc_must_check Err readSecureArea(void *buffer) {
        return c::ncgc_nread_secure_area(&_card, buffer);
    }

    inline __ncgc_must_check Err sendCommand(const uint64_t command, void *const buf, const size_t size, NTRFlags flags, bool flagsAsIs = false) {
        c::ncgc_nflags_t flagst { static_cast<uint32_t>(flags) };
        if (flagsAsIs) {
            return ncgc_nsend_command_as_is(&_card, command, buf, size, flagst);
        } else {
            return ncgc_nsend_command(&_card, command, buf, size, flagst);
        }
    }

    inline __ncgc_must_check Err sendCommand(const uint8_t (&command)[8], void *const buf, const size_t size, NTRFlags flags, bool flagsAsIs = false) {
        return sendCommand(c::cmdBytesToQword(command), buf, size, flags, flagsAsIs);
    }

    inline __ncgc_must_check Err sendWriteCommand(const uint64_t command, void *const buf, const size_t size, NTRFlags flags, bool flagsAsIs = false) {
        c::ncgc_nflags_t flagst { static_cast<uint32_t>(flags) };
        if (flagsAsIs) {
            return ncgc_nsend_write_command_as_is(&_card, command, buf, size, flagst);
        } else {
            return ncgc_nsend_write_command(&_card, command, buf, size, flagst);
        }
    }

    inline __ncgc_must_check Err sendWriteCommand(const uint8_t (&command)[8], void *const buf, const size_t size, NTRFlags flags, bool flagsAsIs = false) {
        return sendWriteCommand(c::cmdBytesToQword(command), buf, size, flags, flagsAsIs);
    }

    inline __ncgc_must_check Err sendSpi(const uint8_t *const command, const size_t command_length,
                                         uint8_t *const response, const size_t response_length) {
        return c::ncgc_nspi_command(&_card, command, command_length, response, response_length);
    }

    inline NTRState state() {
        return static_cast<NTRState>(_card.state);
    }

    inline void state(NTRState state) {
        _card.state = static_cast<c::ncgc_nstate_t>(state);
    }

    inline std::uint32_t gameCode() {
        return _card.hdr.game_code;
    }

    inline std::uint32_t chipId() {
        return _card.raw_chipid;
    }

    inline NTRFlags key1Flags() {
        return _card.hdr.key1_romcnt;
    }

    inline NTRFlags key2Flags() {
        return _card.hdr.key2_romcnt;
    }

    inline c::ncgc_ncard_t& rawState() {
        return _card;
    }
private:
    c::ncgc_ncard_t _card;
};
}
#endif /* NCGCPP_NTRCARD_H */
