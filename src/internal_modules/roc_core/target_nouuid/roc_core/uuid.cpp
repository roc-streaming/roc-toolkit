/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/uuid.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"
#include "roc_core/secure_random.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

// Generates OSF DCE UUID v4, as specified in RFC 4122,
// and returns it in 8-4-4-4-12 text format.
//
// This is the simplest UUID implementation: all bits except
// variant (2 bits) and version (4 bits) are random.
//
// In binary form, UUID is 16 bytes = 128 bits, 6 of which are constant,
// and 122 are random (giving 5.3 * 10^36 possible UUIDs).
//
// In text form, UUID is 36 characters long (32 hex chars + 4 dashes),
// plus terminating zero bye.
bool uuid_generate(char* buf, size_t buf_sz) {
    if (!buf) {
        roc_panic("uuid: buffer is null");
    }
    if (buf_sz < UuidLen + 1) {
        roc_panic("uuid: buffer too small");
    }

    uint8_t bytes[16];
    bool ok = core::secure_random(bytes, ROC_ARRAY_SIZE(bytes));
    if (!ok) {
        return false;
    }

    // Set variant to OSF DCE UUID.
    // 15th character in output string is always '4'.
    bytes[8] &= 0x3F;
    bytes[8] |= 0x80;

    // Set version to 4.
    // 20th character in output string is always '8', '9', 'a', or 'b'.
    bytes[6] &= 0x0F;
    bytes[6] |= 0x40;

    size_t char_pos = 0;
    size_t byte_pos = 0;

    while (char_pos < UuidLen) {
        if (char_pos == 8 || char_pos == 13 || char_pos == 18 || char_pos == 23) {
            buf[char_pos++] = '-';
        } else {
            buf[char_pos++] = "0123456789abcdef"[bytes[byte_pos] >> 4];
            buf[char_pos++] = "0123456789abcdef"[bytes[byte_pos] & 0xF];
            byte_pos++;
        }
    }

    roc_panic_if(byte_pos != ROC_ARRAY_SIZE(bytes));
    roc_panic_if(char_pos != UuidLen);

    buf[UuidLen] = '\0';

    return true;
}

} // namespace core
} // namespace roc
