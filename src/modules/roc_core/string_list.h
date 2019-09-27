/*
 * Copyright (c) 2019 Roc authors
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
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Dynamic list of strings.
class StringList : public NonCopyable<> {
public:
    //! Initialize empty string list.
    explicit StringList(IAllocator& allocator);

    //! Get number of elements.
    size_t size() const;

    //! Get first string.
    //! @returns
    //!  the first string in the list or NULL if it is empty.
    const char* front() const;

    //! Get next string.
    //! @returns
    //!  the first string of the given string or NULL if it is the last string.
    //! @remarks
    //!  @p str should be a pointer returned by front() or nextof(). These
    //!  pointers are invalidated by methods that modify the list.
    const char* nextof(const char* str) const;

    //! Append string to the list.
    //! @remarks
    //!  Reallocates memory if necessary.
    //! @returns
    //!  false if allocation failed.
    bool push_back(const char* str);

    //! Append string to the list if it's not in the list already.
    //! @remarks
    //!  Reallocates memory if necessary.
    //! @returns
    //!  false if allocation failed.
    bool push_back_uniq(const char* str);

    //! Clear the list.
    void clear();

private:
    enum { MinCapacity = 128 };

    bool grow_(size_t size);

    core::Array<char> data_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_LIST_H_
