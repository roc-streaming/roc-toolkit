/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/string_buffer.h
//! @brief String buffer.

#ifndef ROC_CORE_STRING_BUFFER_H_
#define ROC_CORE_STRING_BUFFER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Immutable on-stack string buffer base class.
template <size_t Size> class StringBuffer : public NonCopyable<> {
public:
    //! Get NULL-terminated string.
    const char* c_str() const {
        return buffer;
    }

protected:
    //! Uninitialized buffer.
    //! @remarks
    //!  Should be initialized with a NULL-terminated string in
    //!  derived class constructor.
    char buffer[Size];
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_BUFFER_H_
