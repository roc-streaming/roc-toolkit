/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/string_builder.h
//! @brief String builder.

#ifndef ROC_CORE_STRING_BUILDER_H_
#define ROC_CORE_STRING_BUILDER_H_

#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_buffer.h"

namespace roc {
namespace core {

//! String builder.
//!
//! Allows to incrementally build a string. Doesn't own the string itself, but
//! instead holds a reference to external fixed-size or dynamic buffer.
//
//! Supports "dry run" mode when no actual writing happens. This can be used
//! to calculate the required buffer size before writing.
//!
//! When used with fixed-sized buffer, all methods are signal-safe and hence
//! can be used from a signal handler.
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
    //!
    //! If @p buf is non-NULL, @p bufsz should be non-zero so that buffer
    //! could hold at least zero terminator. Otherwise, error flag is raised
    //! immediately in constructor.
    //!
    //! If @p buf is NULL, @p bufsz may be both zero and non-zero. Use
    //! non-zero to get an error when buffer size is exceeded (like if it was
    //! a real buffer); use zero to disable buffer size checking (there is
    //! no buffer anyway).
    StringBuilder(char* buf, size_t bufsz);

    //! Construct string builder on top of dynamic buffer.
    //!
    //! The builder will write output string into the given buffer. The buffer
    //! will be resized accordingly to the output string size plus terminating
    //! zero byte. The buffer will be always zero-terminated.
    StringBuilder(StringBuffer& buf);

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
    //! @remarks
    //!  Error flag is raised if any of the methods fail, and is cleared
    //!  if an assign* method succeeds.
    bool is_ok() const;

    //! Overwrite result with given string.
    //! If there is not enough space, truncates the string and returns false.
    bool rewrite(const char* str);

    //! Append to result given range.
    //! If there is not enough space, truncates the string and returns false.
    bool append_range(const char* str_begin, const char* str_end);

    //! Append to result given string.
    //! If there is not enough space, truncates the string and returns false.
    bool append_str(const char* str);

    //! Append to result given character.
    //! If there is not enough space, truncates the string and returns false.
    bool append_char(char ch);

    //! Format and append to result given signed integer.
    //! If there is not enough space, truncates the string and returns false.
    bool append_sint(int64_t number, unsigned int base);

    //! Format and append to result given unsigned integer.
    //! If there is not enough space, truncates the string and returns false.
    bool append_uint(uint64_t number, unsigned int base);

private:
    class IBufferWriter {
    public:
        virtual ~IBufferWriter();

        virtual bool is_noop() = 0;
        virtual bool reset() = 0;
        virtual bool grow_by(size_t n_chars) = 0;
        virtual ssize_t extend_by(size_t n_chars) = 0;
        virtual char* write_ptr() = 0;
    };

    class StaticBufferWriter : public IBufferWriter {
    public:
        StaticBufferWriter(char* buf, size_t buf_size);

        virtual bool is_noop();
        virtual bool reset();
        virtual bool grow_by(size_t n_chars);
        virtual ssize_t extend_by(size_t n_chars);
        virtual char* write_ptr();

    private:
        char* const buf_;
        const size_t buf_max_size_;
        size_t buf_cur_size_;
        char* buf_wr_ptr_;
    };

    class DynamicBufferWriter : public IBufferWriter {
    public:
        DynamicBufferWriter(StringBuffer& buf);

        virtual bool is_noop();
        virtual bool reset();
        virtual bool grow_by(size_t n_chars);
        virtual ssize_t extend_by(size_t n_chars);
        virtual char* write_ptr();

    private:
        StringBuffer& buf_;
        char* buf_wr_ptr_;
    };

    void initialize_();
    void reset_();
    bool append_(const char* str, size_t str_size, bool grow);

    Optional<IBufferWriter,
             ROC_MAX(sizeof(StaticBufferWriter), sizeof(DynamicBufferWriter))>
        writer_;

    size_t n_processed_;
    size_t n_written_;

    bool truncation_error_;
    bool write_error_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_BUILDER_H_
