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

#ifndef NCGC_NTRCARD_H
#define NCGC_NTRCARD_H

#include "nocpp.h"

#if !defined(__cplusplus)
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#endif

#include "err.h"
#include "compiler.h"
#include "blowfish.h"

/// A struct wrapping over the raw flags bitfield.
///
/// Try your best not to modify this directly.
typedef struct ncgc_nflags {
    uint32_t flags;
} ncgc_nflags_t;

typedef union {
    uint64_t int_data;
    void *ptr_data;
    void (*fn_data)();
} ncgc_nplatform_data_t;

struct ncgc_ncard;

typedef struct ncgc_nplatform {
    ncgc_nplatform_data_t data;

    /// Platform reset. Called by ncgc_ninit.
    ///
    /// `card->encryption_state` is set to `NCGC_NPREINIT` upon success.
    ///
    /// Returns `NCGC_EOK` on success, or an appropriate error on failure.
    ncgc_err_t __ncgc_must_check (*reset)(struct ncgc_ncard *card);

    /// Sends a command and reads the response.
    ///
    /// Returns `NCGC_EOK` on success, or an appropriate error on failure.
    ncgc_err_t __ncgc_must_check (*send_command)(struct ncgc_ncard *card, uint64_t cmd, uint32_t read_size, void *dest, uint32_t dest_size, ncgc_nflags_t flags);

    /// Sends a write command.
    ///
    /// Returns `NCGC_EOK` on success, or an appropriate error on failure.
    ncgc_err_t __ncgc_must_check (*send_write_command)(struct ncgc_ncard *card, uint64_t cmd, const void *src, uint32_t src_size, ncgc_nflags_t flags);

    /// Sends one byte over the SPI bus, and receives one byte back.
    ///
    /// If `last` is set, SPI CS should be pulled high after the transaction.
    ///
    /// Returns `NCGC_EOK` on success, or an appropriate error on failure.
    ncgc_err_t __ncgc_must_check (*spi_transact)(struct ncgc_ncard *card, uint8_t in, uint8_t *out, bool last);

    void (*io_delay)(uint32_t delay);

    /// Sets the KEY2 registers.
    ///
    /// This function will not be called if `hw_key2` is false.
    void (*seed_key2)(struct ncgc_ncard *card, uint64_t x, uint64_t y);

    /// True if the platform has hardware KEY2 support. In that case, in KEY2 mode, commands are passed to `send_command`
    /// in plaintext. The platform should do the necessary encryption of the command bytes and decryption of data bytes.
    bool hw_key2;
} ncgc_nplatform_t;

typedef enum {
    NCGC_NUNKNOWN = 0,
    NCGC_NPREINIT = 10,
    NCGC_NRAW = 11,
    NCGC_NKEY1 = 12,
    NCGC_NKEY2 = 13
} ncgc_nstate_t;

typedef struct ncgc_ncard {
    /// The chip ID, stored in `ntrcard_init`
    uint32_t raw_chipid;

    struct {
        /// The game code, from the header
        uint32_t game_code;
        /// The KEY1 ROMCNT settings, as in the header
        uint32_t key1_romcnt;
        /// The KEY2 ROMCNT settings, as in the header
        uint32_t key2_romcnt;
    } hdr;

    struct {
        /// The chip ID gotten in KEY1 mode
        uint32_t chipid;
        /// The KEY1 ROMCNT settings, as used
        uint32_t romcnt;
        /// KEY1 Blowfish P array and S boxes
        uint32_t ps[NCGC_NBF_PS_N32];
        /// KEY1 Blowfish key
        uint32_t key[3];
        /// KEY1 nonce iiijjj
        uint32_t ij;
        /// KEY1 counter
        uint32_t k;
        /// KEY1 nonce llll
        uint16_t l;
    } key1;

    ncgc_nstate_t state;

    struct {
        /// The KEY2 seed byte, as in the header
        uint8_t seed_byte;
        /// The KEY2 ROMCNT settings, as used
        uint32_t romcnt;
        /// The KEY2 seed, set in `ntrcard_begin_key1`
        uint32_t mn;
        /// The chip ID gotten in KEY2 mode
        uint32_t chipid;

        /// The KEY2 state X
        uint64_t x;
        /// The KEY2 state Y
        uint64_t y;
    } key2;

    ncgc_nplatform_t platform;
} ncgc_ncard_t;

/// Returns the delay before the response to a KEY1 command (KEY1 gap1)
inline uint16_t ncgc_nflags_predelay(const ncgc_nflags_t flags) { return (uint16_t) (flags.flags & 0x1FFF); }
/// Returns the delay after the response to a KEY1 command (KEY1 gap2)
inline uint16_t ncgc_nflags_postdelay(const ncgc_nflags_t flags) { return (uint16_t) ((flags.flags >> 16) & 0x3F); }
/// Returns true if clock pulses should be sent, and the KEY2 state advanced, during the pre- and post(?)-delays
inline bool ncgc_nflags_delay_pulse_clock(const ncgc_nflags_t flags) { return !!(flags.flags & (1 << 28)); }
/// Returns true if the command is KEY2-encrypted
inline bool ncgc_nflags_key2_command(const ncgc_nflags_t flags) { return (!!(flags.flags & (1 << 22))) && (!!(flags.flags & (1 << 14))); }
/// Returns true if the response is KEY2-encrypted
inline bool ncgc_nflags_key2_data(const ncgc_nflags_t flags) { return (!!(flags.flags & (1 << 13))) && (!!(flags.flags & (1 << 14))); }
/// Returns true if the slower CLK rate should be used (usually for raw commands)
inline bool ncgc_nflags_slow_clock(const ncgc_nflags_t flags) { return (!!(flags.flags & (1 << 27))); }

/// Sets the the delay before the response to a KEY1 command (KEY1 gap1)
inline void ncgc_nflags_set_predelay(ncgc_nflags_t *const flags, const uint16_t predelay) { flags->flags = (flags->flags & 0xFFFFE000) | (predelay & 0x1FFF); }
/// Sets the delay after the response to a KEY1 command (KEY1 gap2)
inline void ncgc_nflags_set_postdelay(ncgc_nflags_t *const flags, const uint16_t postdelay) { flags->flags = (flags->flags & 0xFFC0FFFF) | ((postdelay & 0x3F) << 16); }
/// Set if clock pulses should be sent, and the KEY2 state advanced, during the pre- and post(?)-delays
inline void ncgc_nflags_set_delay_pulse_clock(ncgc_nflags_t *const flags, const bool set) { flags->flags = (flags->flags & ~(1 << 28)) | (set ? (1 << 28) : 0); }
/// Set if the command is KEY2-encrypted
inline void ncgc_nflags_set_key2_command(ncgc_nflags_t *const flags, const bool set) {
    flags->flags = (flags->flags & ~((1 << 22) | (1 << 14))) |
        (set ? (1 << 22) : 0) | ((set || ncgc_nflags_key2_data(*flags)) ? (1 << 14) : 0);
}
/// Set if the response is KEY2-encrypted
inline void ncgc_nflags_set_key2_data(ncgc_nflags_t *const flags, const bool set) {
    flags->flags = (flags->flags & ~((1 << 13) | (1 << 14))) |
        (set ? (1 << 13) : 0) | ((set || ncgc_nflags_key2_command(*flags)) ? (1 << 14) : 0);
}
/// Set if the slower CLK rate should be used (usually for raw commands)
inline void ncgc_nflags_set_slow_clock(ncgc_nflags_t *const flags, const bool set) { flags->flags = (flags->flags & ~(1 << 27)) | (set ? (1 << 27) : 0); }

/// Constructs a `ncgc_nflags_t`.
inline ncgc_nflags_t ncgc_nflags_construct(const uint16_t predelay,
                                           const uint16_t postdelay,
                                           const bool delay_pulse_clock,
                                           const bool key2_command,
                                           const bool key2_data,
                                           const bool slow_clock) {
    ncgc_nflags_t flags;
    ncgc_nflags_set_predelay(&flags, predelay);
    ncgc_nflags_set_postdelay(&flags, postdelay);
    ncgc_nflags_set_delay_pulse_clock(&flags, delay_pulse_clock);
    ncgc_nflags_set_key2_command(&flags, key2_command);
    ncgc_nflags_set_key2_data(&flags, key2_data);
    ncgc_nflags_set_slow_clock(&flags, slow_clock);
    return flags;
}

/// See `ncgc_ninit`.
///
/// If header_first is true, the header is read before the chip ID. Otherwise,
/// the chip ID is read first.
ncgc_err_t __ncgc_must_check ncgc_ninit_order(ncgc_ncard_t *card, void *buf, bool header_first);

/// Initialises the card slot and card, optionally reading the header into `buf`, if `buf` is not null.
/// If specified, `buf` should be at least 0x1000 bytes.
///
/// Returns `NCGC_EOK` on success, or the error returned by the platform reset function,
/// if the platform reset function fails.
inline ncgc_err_t __ncgc_must_check ncgc_ninit(ncgc_ncard_t *const card, void *const buf) {
    return ncgc_ninit_order(card, buf, false);
}

/// Sets up the blowfish state based on the game code in the header, and the provided initial P array/S boxes.
void ncgc_nsetup_blowfish(ncgc_ncard_t* card, const uint8_t ps[NCGC_NBF_PS_N32*4]);

/// Sets up the blowfish state with the provided initial P array/S boxes.
inline void ncgc_nsetup_blowfish_as_is(ncgc_ncard_t* card, const uint8_t ps[NCGC_NBF_PS_N32*4]) {
    memcpy(card->key1.ps, ps, sizeof(card->key1.ps));
}

/// Brings the card into KEY1 mode.
///
/// The card encryption state must be RAW when this function is called. The blowfish P array and S boxes need to be
/// initialised before this function is called, otherwise it will likely fail with error -2.
///
/// Returns `NCGC_EOK` on success, `NCGC_ECSTATE` if the encryption state is not currently RAW,
/// `NCGC_ECRESP` if the KEY1 CHIPID command result does not match the raw chip ID stored in `card`, or
/// the error returned by the platform, if a platform function fails.
ncgc_err_t __ncgc_must_check ncgc_nbegin_key1(ncgc_ncard_t *card);

/// Reads the secure area. `dest` must be at least 0x4000 bytes.
///
/// Returns `NCGC_EOK` on success, `NCGC_ECSTATE` if the encryption state is not currently KEY1, or
/// the error returned by the platform, if a platform function fails. The secure area will be read into `dest`.
ncgc_err_t __ncgc_must_check ncgc_nread_secure_area(ncgc_ncard_t *card, void *dest);

/// Brings the card into KEY2 mode.
///
/// The card encryption state must be KEY1 when this function is called.
///
/// Returns `NCGC_EOK` on success, `NCGC_ECSTATE` if the encryption state is not currently KEY1,
/// `NCGC_ECRESP` if the KEY1 CHIPID command result does not match the raw chip ID stored in `card`, or
/// the error returned by the platform, if a platform function fails.
ncgc_err_t __ncgc_must_check ncgc_nbegin_key2(ncgc_ncard_t *card);

/// Reads the card data using the NTR 0xB7 command into `buf`, if `buf` is not NULL.
///
/// Returns `NCGC_EOK` on success, `NCGC_ECSTATE` if the encryption state is not currently KEY2, or
/// the error returned by the platform, if a platform function fails.
ncgc_err_t __ncgc_must_check ncgc_nread_data(ncgc_ncard_t *card, uint32_t address, void *buf, size_t size);

/// Sends a command `command`, with flags `flags` modified accordingly for the card state, to the card, and reads the
/// response of size `size` to a buffer `buf`, if `buf` is not NULL.
///
/// Returns the error code from the first platform function that fails, if any, or `NCGC_EOK` on success.
inline ncgc_err_t __ncgc_must_check ncgc_nsend_command(ncgc_ncard_t *const card, const uint64_t command, void *const buf,
                                                    const size_t size, ncgc_nflags_t flags) {
    if (card->state == NCGC_NKEY2) {
        ncgc_nflags_set_key2_command(&flags, true);
        ncgc_nflags_set_key2_data(&flags, true);
    }
    return card->platform.send_command(card, command, size, buf, buf ? size : 0, flags);
}

/// Sends a command `command`, with flags `flags` used as-is, to the card, and reads the
/// response of size `size` to a buffer `buf`, if `buf` is not NULL.
///
/// Returns the error code from the first platform function that fails, if any, or `NCGC_EOK` on success.
inline ncgc_err_t __ncgc_must_check ncgc_nsend_command_as_is(ncgc_ncard_t *const card, const uint64_t command, void *const buf,
                                                    const size_t size, ncgc_nflags_t flags) {
    return card->platform.send_command(card, command, size, buf, buf ? size : 0, flags);
}

/// Sends a command `command`, with flags `flags` modified accordingly for the card state, to the card, and then writes
/// the data of size `size` to the card.
///
/// Returns the error code from the first platform function that fails, if any, or `NCGC_EOK` on success.
inline ncgc_err_t __ncgc_must_check ncgc_nsend_write_command(ncgc_ncard_t *const card, const uint64_t command, const void *const buf,
                                                    const size_t size, ncgc_nflags_t flags) {
    if (card->state == NCGC_NKEY2) {
        ncgc_nflags_set_key2_command(&flags, true);
        ncgc_nflags_set_key2_data(&flags, true);
    }
    return card->platform.send_write_command(card, command, buf, size, flags);
}

/// Sends a command `command`, with flags `flags` used as-is, to the card, and then writes the data of size
/// `size` to the card.
///
/// Returns the error code from the first platform function that fails, if any, or `NCGC_EOK` on success.
inline ncgc_err_t __ncgc_must_check ncgc_nsend_write_command_as_is(ncgc_ncard_t *const card, const uint64_t command, const void *const buf,
                                                    const size_t size, ncgc_nflags_t flags) {
    return card->platform.send_write_command(card, command, buf, size, flags);
}

/// Sends an SPI command `command` of length `command_length`, then receives a response of length `response_length`
/// into `response`, if `response` is not NULL.
///
/// Returns the error code from the first platform function that fails, if any, or `NCGC_EOK` on success.
ncgc_err_t __ncgc_must_check ncgc_nspi_command(ncgc_ncard_t *card, const uint8_t *command, size_t command_length,
                                            uint8_t *response, size_t response_length);

#if defined(NCGC_PLATFORM_NTR)
    #include "platform_ntr.h"
#elif defined(NCGC_PLATFORM_CTR)
    #include "platform_ctr.h"
#elif !defined(NCGC_PLATFORM_TEST)
    #error No NCGC platform defined.
#endif
#endif /* NCGC_NTRCARD_H */
