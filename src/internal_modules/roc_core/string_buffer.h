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
    explicit StringBuffer(IArena& arena);

    //! Check if buffer is empty.
    bool is_empty() const;

    //! Get string length, excluding terminating zero.
    size_t len() const;

    //! Get zero-terminated string.
    const char* c_str() const;

    //! Set buffer to empty string.
    void clear();

    //! Copy given string into buffer.
    //! @p str should be zero-terminated.
    //! @returns
    //!  false if allocation failed.
    ROC_ATTR_NODISCARD bool assign(const char* str);

    //! Copy given range into buffer.
    //! Buffer will be automatically zero-terminated.
    //! @returns
    //!  false if allocation failed.
    ROC_ATTR_NODISCARD bool assign(const char* str_begin, const char* str_end);

    //! Extend buffer by requested number of characters.
    //! @remarks
    //!  Characters are appended to the buffer and filled with zeros.
    //!  It's the caller responsibility to fill them.
    //! @returns
    //!  NULL if allocation failed.
    ROC_ATTR_NODISCARD char* extend(size_t n_chars);

    //! Grow capacity to be able to hold desired number of characters.
    //! Capacity is increased linearly.
    //! @returns
    //!  false if allocation failed.
    ROC_ATTR_NODISCARD bool grow(size_t desired_len);

    //! Grow capacity to be able to hold desired number of characters.
    //! Capacity is increased exponentially.
    //! @returns
    //!  false if allocation failed.
    ROC_ATTR_NODISCARD bool grow_exp(size_t desired_len);

private:
    Array<char, 32> data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_BUFFER_H_
