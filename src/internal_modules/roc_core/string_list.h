/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/string_list.h
//! @brief Dynamic list of strings.

#ifndef ROC_CORE_STRING_LIST_H_
#define ROC_CORE_STRING_LIST_H_

#include "roc_core/array.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Dynamic list of strings.
//!
//! Strings are stored in a continuous dynamically-growing array.
//! Each string is stored in a block with a header which holds offsets to previous
//! and next blocks, forming a linked list. This allows implementing bidirectional
//! iteration and sorting. Using offsets instead of pointers is needed to avoid
//! pointer invalidation after reallocation.
//!
//! @code
//!  ++--------+--------+---------++-----------
//!  || Header | string | padding || Header ...
//!  ++--------+--------+---------++-----------
//! @endcode
class StringList : public NonCopyable<> {
public:
    //! Initialize empty string list.
    explicit StringList(IArena& arena);

    //! Get number of elements.
    size_t size() const;

    //! Check if list is empty.
    bool is_empty() const;

    //! Get first string.
    //! @returns
    //!  the first string in the list or NULL if it is empty.
    const char* front() const;

    //! Get last string.
    //! @returns
    //!  the last string in the list or NULL if it is empty.
    const char* back() const;

    //! Get next string.
    //! @returns
    //!  the first string of the given string or NULL if it is the last string.
    //! @remarks
    //!  @p str should be a pointer returned by front(), nextof(), or prevof().
    //!  These pointers are invalidated by methods that modify the list.
    const char* nextof(const char* str) const;

    //! Get previous string.
    //! @returns
    //!  the last string of the given string or NULL if it is the first string.
    //! @remarks
    //!  @p str should be a pointer returned by back(), nextof(), or prevof().
    //!  These pointers are invalidated by methods that modify the list.
    const char* prevof(const char* str) const;

    //! Clear the list.
    void clear();

    //! Append string to the list.
    //! @remarks
    //!  Reallocates memory if necessary.
    //! @returns
    //!  false if allocation failed.
    ROC_ATTR_NODISCARD bool push_back(const char* str);

    //! Append string from a range to the list.
    //! @remarks
    //!  Reallocates memory if necessary.
    //! @returns
    //!  false if allocation failed.
    ROC_ATTR_NODISCARD bool push_back(const char* str_begin, const char* str_end);

    //! Remove string from end of the list.
    //! @returns
    //!  false if deallocation failed.
    ROC_ATTR_NODISCARD bool pop_back();

    //! Find string in the list.
    //! @returns
    //!  the string in the list or NULL if it is not found.
    const char* find(const char* str);

    //! Find string in the list.
    //! @returns
    //!  the string in the list or NULL if it is not found.
    const char* find(const char* str_begin, const char* str_end);

    //! String comparison algorithm.
    enum Order {
        //! Sort in lexicographic order.
        //! Assumes ASCII.
        OrderLexical,
        //! Sort in natural order.
        //! Assumes ASCII.
        OrderNatural,
    };

    //! Sort list of strings according to specified order.
    void sort(Order order);

private:
    enum { MinCapacity = 128 };

    typedef uint32_t offset_t;

    struct Header {
        // offsets of next and previous elements in memory
        offset_t next_off;
        offset_t prev_off;
        // len of this block, including header and padding
        offset_t blk_len;
        // null-terminated string
        char str[];
    };

    void swap_(offset_t x_off, Header* x_header, offset_t y_off, Header* y_header);

    offset_t to_offset_(const Header* header) const;
    const Header* from_offset_(offset_t off) const;
    Header* from_offset_(offset_t off);

    void check_member_(const char* str) const;

    bool grow_(size_t size);

    core::Array<char> memory_;
    offset_t head_off_;
    offset_t tail_off_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_LIST_H_
