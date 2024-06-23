/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/string_builder.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

StringBuilder::IBufferWriter::~IBufferWriter() {
}

StringBuilder::StaticBufferWriter::StaticBufferWriter(char* buf, size_t buf_size)
    : buf_(buf)
    , buf_max_size_(buf_size)
    , buf_cur_size_(1)
    , buf_wr_ptr_(NULL) {
}

bool StringBuilder::StaticBufferWriter::reset() {
    if (buf_ && buf_max_size_ == 0) {
        // error: buffer isn't null, but there is no space for zero terminator
        return false;
    }

    buf_cur_size_ = 1;
    buf_wr_ptr_ = NULL;

    if (buf_) {
        buf_[0] = '\0';
    }

    return true;
}

bool StringBuilder::StaticBufferWriter::is_noop() {
    return buf_ == NULL;
}

bool StringBuilder::StaticBufferWriter::grow_by(size_t) {
    // ignore
    return true;
}

ssize_t StringBuilder::StaticBufferWriter::extend_by(size_t n_chars) {
    if (buf_ && buf_max_size_ == 0) {
        // error: buffer isn't null, but there is no space for zero terminator
        return -1;
    }

    if (!buf_ && buf_max_size_ == 0) {
        // special case: when buffer is null, zero buffer size means no limit
        return (ssize_t)n_chars;
    }

    const size_t max_chars = buf_max_size_ - buf_cur_size_;
    if (n_chars > max_chars) {
        n_chars = max_chars;
    }

    if (buf_) {
        buf_wr_ptr_ = buf_ + buf_cur_size_ - 1;
    }

    buf_cur_size_ += n_chars;

    if (buf_) {
        buf_[buf_cur_size_ - 1] = '\0';
    }

    return (ssize_t)n_chars;
}

char* StringBuilder::StaticBufferWriter::write_ptr() {
    return buf_wr_ptr_;
}

StringBuilder::DynamicBufferWriter::DynamicBufferWriter(StringBuffer& buf)
    : buf_(buf)
    , buf_wr_ptr_(NULL) {
}

bool StringBuilder::DynamicBufferWriter::is_noop() {
    return false;
}

bool StringBuilder::DynamicBufferWriter::reset() {
    buf_.clear();
    buf_wr_ptr_ = NULL;
    return true;
}

bool StringBuilder::DynamicBufferWriter::grow_by(size_t n_chars) {
    return buf_.grow_exp(buf_.len() + n_chars);
}

ssize_t StringBuilder::DynamicBufferWriter::extend_by(size_t n_chars) {
    buf_wr_ptr_ = buf_.extend(n_chars);
    return buf_wr_ptr_ ? (ssize_t)n_chars : -1;
}

char* StringBuilder::DynamicBufferWriter::write_ptr() {
    return buf_wr_ptr_;
}

StringBuilder::StringBuilder(char* buf, size_t bufsz) {
    writer_.reset(new (writer_) StaticBufferWriter(buf, bufsz));
    initialize_();
}

StringBuilder::StringBuilder(StringBuffer& buf) {
    writer_.reset(new (writer_) DynamicBufferWriter(buf));
    initialize_();
}

size_t StringBuilder::needed_size() const {
    return n_processed_ + 1;
}

size_t StringBuilder::actual_size() const {
    if (writer_->is_noop() || write_error_) {
        return 0;
    }

    return n_written_ + 1;
}

bool StringBuilder::is_ok() const {
    return !truncation_error_ && !write_error_;
}

bool StringBuilder::rewrite(const char* str) {
    roc_panic_if_not(str);

    reset_();
    return append_(str, strlen(str), false);
}

bool StringBuilder::append_range(const char* str_begin, const char* str_end) {
    roc_panic_if_not(str_begin);
    roc_panic_if_not(str_begin <= str_end);

    return append_(str_begin, size_t(str_end - str_begin), true);
}

bool StringBuilder::append_str(const char* str) {
    roc_panic_if_not(str);

    return append_(str, strlen(str), true);
}

bool StringBuilder::append_char(char ch) {
    return append_(&ch, 1, true);
}

bool StringBuilder::append_sint(int64_t number, unsigned int base) {
    roc_panic_if_not(base >= 2 && base <= 16);

    if (number < 0) {
        append_("-", 1, true);
        number = -number;
    }

    return append_uint((uint64_t)number, base);
}

bool StringBuilder::append_uint(uint64_t number, unsigned int base) {
    roc_panic_if_not(base >= 2 && base <= 16);

    // we can't use snprintf() because it's not signal-safe
    // we can't use itoa() because it's non-standard

    char tmp[128]; // 128 is enough for any base in case of 64-bit ints
    size_t tmp_pos = sizeof(tmp) - 1;
    do {
        tmp[tmp_pos] = "0123456789ABCDEF"[number % base];
        tmp_pos--;
        number = number / base;
    } while (number > 0);

    return append_(tmp + tmp_pos + 1, sizeof(tmp) - tmp_pos - 1, true);
}

void StringBuilder::initialize_() {
    n_processed_ = 0;
    n_written_ = 0;

    truncation_error_ = false;
    write_error_ = false;

    reset_();
}

void StringBuilder::reset_() {
    n_processed_ = 0;
    n_written_ = 0;

    if (!write_error_) {
        if (!writer_->reset()) {
            write_error_ = true;
            return;
        }

        truncation_error_ = false;
    }
}

bool StringBuilder::append_(const char* str, size_t str_size, bool grow) {
    roc_panic_if_not(str);

    n_processed_ += str_size;

    if (!write_error_) {
        if (grow) {
            if (!writer_->grow_by(str_size)) {
                write_error_ = true;
                return is_ok();
            }
        }

        if (str_size != 0) {
            const ssize_t write_size = writer_->extend_by(str_size);

            if (write_size < 0) {
                write_error_ = true;
                return is_ok();
            }

            if (write_size > 0) {
                if (char* write_ptr = writer_->write_ptr()) {
                    memcpy(write_ptr, str, (size_t)write_size);
                    n_written_ += (size_t)write_size;
                }
            }

            if ((size_t)write_size < str_size) {
                truncation_error_ = true;
            }
        }
    }

    return is_ok();
}

} // namespace core
} // namespace roc
