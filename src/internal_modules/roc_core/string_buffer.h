/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/string_buffer.h
//! @brief String buffer.

#ifndef ROC_CORE_STRING_BUFFER_H_
#define ROC_CORE_STRING_BUFFER_H_

#include "roc_core/array.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! String buffer.
//!
//! Dynamic array storing zero-terminated string. Works on top of Array,
//! but guarantees that the string is always zero-terminated.
//!
//! @tparam EmbeddedCapacity is the same as for Array.
class StringBuffer : public NonCopyable<> {
public:
    //! Initialize empty buffer.
    explicit StringBuffer(IArena& arena)
        : data_(arena) {
        clear();
    }

    //! Check if buffer is empty.
    bool is_empty() const {
        return len() == 0;
    }

    //! Get string length, excluding terminating zero.
    size_t len() const {
        return data_.size() - 1;
    }

    //! Get zero-terminated string.
    const char* c_str() const {
        return data_.data();
    }

    //! Set buffer to empty string.
    void clear() {
        if (!data_.resize(1)) {
            roc_panic("string buffer: allocation failed");
        }
        data_[0] = '\0';
    }

    //! Copy given string into buffer.
    //! @p str should be zero-terminated.
    ROC_ATTR_NODISCARD bool assign(const char* str) {
        if (!str) {
            roc_panic("string buffer: null pointer");
        }

        return assign(str, str + strlen(str));
    }

    //! Copy given range into buffer.
    //! Buffer will be automatically zero-terminated.
    ROC_ATTR_NODISCARD bool assign(const char* str_begin, const char* str_end) {
        if (!str_begin || !str_end) {
            roc_panic("string buffer: null pointer");
        }
        if (str_begin > str_end) {
            roc_panic("string buffer: invalid range");
        }

        const size_t str_sz = size_t(str_end - str_begin);

        if (!data_.resize(str_sz + 1)) {
            clear();
            return false;
        }

        if (str_sz != 0) {
            memcpy(data_.data(), str_begin, str_sz);
        }
        data_[str_sz] = '\0';

        return true;
    }

    //! Extend buffer by requested number of characters.
    //! @remarks
    //!  Characters are appended to the buffer and filled with zeros.
    //!  It's the caller responsibility to fill them.
    char* extend(size_t n_chars) {
        const size_t orig_sz = data_.size();

        if (n_chars > 0) {
            if (!data_.resize(orig_sz + n_chars)) {
                clear();
                return NULL;
            }
        }

        return data_.data() + orig_sz - 1;
    }

    //! Grow capacity to be able to hold desired number of characters.
    //! Capacity is increased linearly.
    ROC_ATTR_NODISCARD bool grow(size_t desired_len) {
        return data_.grow(desired_len + 1);
    }

    //! Grow capacity to be able to hold desired number of characters.
    //! Capacity is increased exponentionally.
    ROC_ATTR_NODISCARD bool grow_exp(size_t desired_len) {
        return data_.grow_exp(desired_len + 1);
    }

private:
    Array<char, 32> data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_BUFFER_H_
