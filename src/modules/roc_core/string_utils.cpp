/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/string_utils.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

bool copy_str(char* dst, size_t dst_size, const char* src_begin, const char* src_end) {
    roc_panic_if_not(dst);
    roc_panic_if_not(src_begin);
    roc_panic_if_not(src_begin <= src_end);

    if (dst_size < 1) {
        return false;
    }

    const size_t src_size = size_t(src_end - src_begin);

    size_t copy_size = src_size;
    if (copy_size > dst_size - 1) {
        copy_size = dst_size - 1;
    }

    if (copy_size != 0) {
        memcpy(dst, src_begin, copy_size);
    }
    dst[copy_size] = '\0';

    return (copy_size == src_size);
}

bool copy_str(char* dst, size_t dst_size, const char* src) {
    roc_panic_if_not(src);

    return copy_str(dst, dst_size, src, src + strlen(src));
}

bool append_str(char* dst, size_t dst_size, const char* src_begin, const char* src_end) {
    roc_panic_if_not(dst);

    if (dst_size < 1) {
        return false;
    }

    const size_t dst_len = strlen(dst);

    return copy_str(dst + dst_len, dst_size - dst_len, src_begin, src_end);
}

bool append_str(char* dst, size_t dst_size, const char* src) {
    roc_panic_if_not(src);

    return append_str(dst, dst_size, src, src + strlen(src));
}

bool append_uint(char* dst, size_t dst_size, uint64_t number, unsigned int base) {
    roc_panic_if_not(base >= 2 && base <= 16);

    // we can't use snprintf() because it's not signal-safe
    // we can't use itoa() because it's non-standard

    char tmp[128]; // 128 is enough for any base in case of 64-bit ints
    size_t tmp_pos = sizeof(tmp) - 1;
    do {
        tmp[tmp_pos] = "0123456789abcdef"[number % base];
        tmp_pos--;
        number = number / base;
    } while (number > 0);

    return append_str(dst, dst_size, tmp + tmp_pos + 1, tmp + sizeof(tmp));
}

} // namespace core
} // namespace roc
