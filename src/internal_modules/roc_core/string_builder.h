/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/string_builder.h
//! @brief String builder.

#ifndef ROC_CORE_STRING_BUILDER_H_
#define ROC_CORE_STRING_BUILDER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! String builder.
//!
//! Allows to incrementally build a string. Doesn't own the string itself, but
//! insetead holds a reference to external fixed-size or dynamic array.
//
//! Supports "dry run" mode when no actual writing happens. This can be used
//! to calculate the required buffer size before writing.
//!
//! When used with fixed-sized buffer, all methods are signal-safe and thus
//! can be used from signal handler.
class StringBuilder : public NonCopyable<> {
public:
    //! Construct string builder on top of fixed-size buffer.
    //!
    //! The builder will write output string into the given buffer.
    //! If the output buffer is too small, the output string is truncated
    //! and error flag is set. If the output buffer has at least one byte,
    //! it will be always zero-terminated, even if truncation occurred.
    //!
    //! @p buf may be NULL. In this case, nothing will be written, but
    //! needed_size() will be still calculated.
    StringBuilder(char* buf, size_t bufsz)
        : buf_(buf)
        , buf_size_(bufsz)
        , array_(NULL)
        , array_resize_(NULL) {
        init_();
    }

    //! Construct string builder on top of dynamic array.
    //!
    //! The builder will write output string into the given array. The array
    //! will be resized accordingly to the output string size plus terminating
    //! zero byte. The array will be always zero-terminated.
    template <class Array>
    StringBuilder(Array& array)
        : buf_(array.data())
        , buf_size_(array.size())
        , array_(&array)
        , array_resize_(&StringBuilder::array_resize_func_<Array>) {
        init_();
    }

    //! Get number of bytes required to store the output string.
    //! Includes terminating zero byte.
    //!
    //! @remarks
    //!  If there is non-NULL output buffer, and no error occurred, this size
    //!  is equal to actual_size(). Otherwise it may be larger.
    size_t needed_size() const;

    //! Get number of bytes actually written to the output string.
    //! Includes terminating zero byte.
    size_t actual_size() const;

    //! Check for errors.
    //!
    //! @remark
    //!  Error flag is raised if any of the merhods fail, and is resetted
    //!  if a set* method succeedes.
    bool ok() const;

    //! Override result with given string.
    //! If there is not enough space, truncates the string and returns false.
    bool set_str(const char* str);

    //! Override result with given string.
    //! If there is not enough space, truncates the string and returns false.
    bool set_str_range(const char* str_begin, const char* str_end);

    //! Append to result given string.
    //! If there is not enough space, truncates the string and returns false.
    bool append_str(const char* str);

    //! Append to result given string.
    //! If there is not enough space, truncates the string and returns false.
    bool append_str_range(const char* str_begin, const char* str_end);

    //! Append to result given character..
    //! If there is not enough space, truncates the string and returns false.
    bool append_char(char ch);

    //! Format and append to result given number.
    //! If there is not enough space, truncates the string and returns false.
    bool append_uint(uint64_t number, unsigned int base);

private:
    typedef char* (*ResizeFunc)(void* array, size_t size, bool exp);

    template <class Array>
    static char* array_resize_func_(void* array, size_t size, bool exp) {
        if (exp) {
            if (!static_cast<Array*>(array)->grow_exp(size)) {
                return NULL;
            }
        }
        if (!static_cast<Array*>(array)->resize(size)) {
            return NULL;
        }
        return static_cast<Array*>(array)->data();
    }

    void init_();
    void reset_();

    bool append_imp_(const char* str, size_t str_size, bool exp);
    size_t request_append_(size_t size, bool exp);

    char* buf_;
    size_t buf_size_;

    size_t output_pos_;
    size_t input_pos_;

    bool ok_;

    void* array_;
    ResizeFunc array_resize_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_BUILDER_H_
