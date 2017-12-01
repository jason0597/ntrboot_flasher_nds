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

#include "../include/ncgc/err.h"

const char *ncgc_err_desc(const ncgc_err_t err) {
    switch (err) {
        case NCGC_EOK:
            return "No error occurred.";
        case NCGC_EUNSUP:
            return "The platform does not support the operation requested.";
        case NCGC_ECSTATE:
            return "The card is in the wrong state.";
        case NCGC_ECRESP:
            return "The card's response was unexpected.";
        case NCGC_ECMISSING:
            return "The card is missing or has disappeared.";
        case NCGC_EHERR:
            return "A hardware error occured.";
        case NCGC_EUNK:
            return "An unknown error occured.";
        default:
            return "Unknown error code.";
    }
}
