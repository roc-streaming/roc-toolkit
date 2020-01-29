/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/string_builder.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

size_t StringBuilder::needed_size() const {
    return input_pos_ + 1;
}

size_t StringBuilder::actual_size() const {
    if (!buf_ || buf_size_ == 0) {
        return 0;
    }

    return output_pos_ + 1;
}

bool StringBuilder::ok() const {
    return ok_;
}

bool StringBuilder::set_str(const char* str) {
    roc_panic_if_not(str);

    reset_();
    return append_imp_(str, strlen(str), false);
}

bool StringBuilder::set_str_range(const char* str_begin, const char* str_end) {
    roc_panic_if_not(str_begin);
    roc_panic_if_not(str_begin <= str_end);

    reset_();
    return append_imp_(str_begin, size_t(str_end - str_begin), false);
}

bool StringBuilder::append_str(const char* str) {
    roc_panic_if_not(str);

    return append_imp_(str, strlen(str), true);
}

bool StringBuilder::append_str_range(const char* str_begin, const char* str_end) {
    roc_panic_if_not(str_begin);
    roc_panic_if_not(str_begin <= str_end);

    return append_imp_(str_begin, size_t(str_end - str_begin), true);
}

bool StringBuilder::append_char(char ch) {
    return append_imp_(&ch, 1, true);
}

// we can't use snprintf() because it's not signal-safe
// we can't use itoa() because it's non-standard
bool StringBuilder::append_uint(uint64_t number, unsigned int base) {
    roc_panic_if_not(base >= 2 && base <= 16);

    char tmp[128]; // 128 is enough for any base in case of 64-bit ints
    size_t tmp_pos = sizeof(tmp) - 1;
    do {
        tmp[tmp_pos] = "0123456789abcdef"[number % base];
        tmp_pos--;
        number = number / base;
    } while (number > 0);

    return append_imp_(tmp + tmp_pos + 1, sizeof(tmp) - tmp_pos - 1, true);
}

void StringBuilder::init_() {
    ok_ = true;
    reset_();
    append_imp_("", 0, false);
}

void StringBuilder::reset_() {
    if (buf_) {
        ok_ = true;
    }

    output_pos_ = 0;
    input_pos_ = 0;

    if (array_) {
        buf_ = array_resize_(array_, 0, false);
        buf_size_ = 0;
    }
}

bool StringBuilder::append_imp_(const char* str, size_t str_size, bool exp) {
    const size_t copy_size = request_append_(str_size + 1, exp);

    if (copy_size > 0) {
        if (copy_size > 1) {
            memcpy(buf_ + output_pos_, str, copy_size - 1);
        }
        buf_[output_pos_ + copy_size - 1] = '\0';
        output_pos_ += copy_size - 1;
    }

    input_pos_ += str_size;

    return ok_;
}

size_t StringBuilder::request_append_(size_t size, bool exp) {
    if (array_) {
        buf_ = array_resize_(array_, output_pos_ + size, exp);
        buf_size_ = output_pos_ + size;

        if (!buf_) {
            ok_ = false;
        }
    }

    if (!buf_) {
        return 0;
    }

    if (output_pos_ + size > buf_size_) {
        ok_ = false;
        return buf_size_ - output_pos_;
    }

    return size;
}

} // namespace core
} // namespace roc
