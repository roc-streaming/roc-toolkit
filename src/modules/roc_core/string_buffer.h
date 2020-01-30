/*
 * Copyright (c) 2020 Roc authors
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
//! @tparam EmbedSize is the same as for Array.
template <size_t EmbedSize = 0> class StringBuffer : public NonCopyable<> {
public:
    //! Initialize empty buffer.
    explicit StringBuffer(IAllocator& allocator)
        : array_(allocator) {
        clear();
    }

    //! Underlying array type.
    typedef Array<char, EmbedSize ? EmbedSize : 1> ArrayType;

    //! Get reference to underlying array.
    //! The user is responsible to keep array of the exact size
    //! as the containing string plus terminating zero byte.
    ArrayType& raw_buf() {
        return array_;
    }

    //! Check if buffer is empty.
    bool is_empty() const {
        return len() == 0;
    }

    //! Get string length, excluding terminating zero.
    size_t len() const {
        return array_.size() - 1;
    }

    //! Get zero-terminated string.
    const char* c_str() const {
        return array_.data();
    }

    //! Set buffer to empty string.
    void clear() {
        array_.resize(1);
        array_[0] = '\0';
    }

    //! Copy given string into buffer.
    //! @p str should be zero-terminated.
    bool set_str(const char* str) {
        if (!str) {
            roc_panic("string buffer: str is null");
        }

        return set_buf(str, strlen(str));
    }

    //! Copy given string into buffer.
    //! @p str should NOT be zero-terminated.
    //! Buffer WILL be zero-terminated.
    bool set_buf(const char* buf, size_t bufsz) {
        if (!buf) {
            roc_panic("string buffer: buf is null");
        }

        if (buf + bufsz < buf) {
            roc_panic("string buffer: bufsz out of bounds");
        }

        if (!array_.resize(bufsz + 1)) {
            clear();
            return false;
        }

        if (bufsz != 0) {
            memcpy(array_.data(), buf, bufsz);
        }
        array_[bufsz] = '\0';

        return true;
    }

    //! Grow buffer capacity.
    bool grow(size_t capacity) {
        return array_.grow(capacity);
    }

private:
    ArrayType array_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_BUFFER_H_
