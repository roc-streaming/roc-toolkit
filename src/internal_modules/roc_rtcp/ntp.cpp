/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/ntp.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtcp {

packet::ntp_timestamp_t ntp_clamp_64(packet::ntp_timestamp_t value,
                                     packet::ntp_timestamp_t max_value) {
    if (value > max_value) {
        value = max_value;
    }
    return value;
}

packet::ntp_timestamp_t ntp_clamp_32(packet::ntp_timestamp_t value,
                                     packet::ntp_timestamp_t max_value) {
    // Truncate low 16 bits with rounding.
    value += 0x8000;
    value &= 0xFFFFFFFFFFFF0000;
    // Saturate to maximum.
    if (value > (max_value & 0xFFFFFFFFFFFF0000)) {
        value = max_value & 0xFFFFFFFFFFFF0000;
    }
    // Truncate high 16 bits.
    value &= 0x0000FFFFFFFF0000;
    return value;
}

packet::ntp_timestamp_t ntp_extend(packet::ntp_timestamp_t base,
                                   packet::ntp_timestamp_t value) {
    roc_panic_if_msg(value & 0xFFFF00000000FFFF, "value should have only middle 32 bits");

    // value extended with high 16 bits from base
    const packet::ntp_timestamp_t extended_value = (base & 0xFFFF000000000000) | value;
    // another candidate: same, but assuming that middle 32 bits of
    // base has wrapped forward after value was stored
    const packet::ntp_timestamp_t wrapped_forward =
        ((base - 0x0001000000000000) & 0xFFFF000000000000) | value;
    // another candidate: same, but assuming that middle 32 bits of
    // base has wrapped backward after value was stored
    const packet::ntp_timestamp_t wrapped_backward =
        ((base + 0x0001000000000000) & 0xFFFF000000000000) | value;

    // choose candidate that is closer to base
    if (std::abs(int64_t(extended_value - base))
            <= std::abs(int64_t(wrapped_forward - base))
        && std::abs(int64_t(extended_value - base))
            <= std::abs(int64_t(wrapped_backward - base))) {
        return extended_value;
    }
    if (std::abs(int64_t(wrapped_forward - base))
        <= std::abs(int64_t(wrapped_backward - base))) {
        return wrapped_forward;
    }
    return wrapped_backward;
}

} // namespace rtcp
} // namespace roc
