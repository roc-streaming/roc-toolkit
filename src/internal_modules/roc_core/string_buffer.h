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
#include "roc_core/iallocator.h"
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
template <size_t EmbeddedCapacity = 0> class StringBuffer : public NonCopyable<> {
public:
    //! Initialize empty buffer.
    explicit StringBuffer(IAllocator& allocator)
        : data_(allocator) {
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
        data_.resize(1);
        data_[0] = '\0';
    }

    //! Copy given string into buffer.
    //! @p str should be zero-terminated.
    bool assign(const char* str) {
        roc_panic_if_not(str);

        return assign_range(str, str + strlen(str));
    }

    //! Copy given range into buffer.
    //! Buffer will be automatically zero-terminated.
    bool assign_range(const char* str_begin, size_t str_size) {
        roc_panic_if_not(str_begin);

        return assign_range(str_begin, str_begin + str_size);
    }

    //! Copy given range into buffer.
    //! Buffer will be automatically zero-terminated.
    bool assign_range(const char* str_begin, const char* str_end) {
        roc_panic_if_not(str_begin);
        roc_panic_if_not(str_begin <= str_end);

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
        roc_panic_if_not(n_chars > 0);

        const size_t orig_sz = data_.size();

        if (!data_.resize(orig_sz + n_chars)) {
            clear();
            return NULL;
        }

        return data_.data() + orig_sz - 1;
    }

    //! Grow capacity to be able to hold desired number of characters.
    //! Capacity is increased linearly.
    bool grow(size_t desired_len) {
        return data_.grow(desired_len + 1);
    }

    //! Grow capacity to be able to hold desired number of characters.
    //! Capacity is increased exponentionally.
    bool grow_exp(size_t desired_len) {
        return data_.grow_exp(desired_len + 1);
    }

private:
    Array<char, EmbeddedCapacity> data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_BUFFER_H_
