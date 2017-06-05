/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ibuffer.h
//! @brief Buffer interface.

#ifndef ROC_CORE_IBUFFER_H_
#define ROC_CORE_IBUFFER_H_

#include "roc_core/refcnt.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Buffer interface.
//! @tparam T specifies element type.
template <class T> class IBuffer : public RefCnt {
public:
    virtual ~IBuffer() {
    }

    //! Get buffer data.
    virtual T* data() = 0;

    //! Get buffer data.
    virtual const T* data() const = 0;

    //! Get maximum allowed number of elements.
    virtual size_t max_size() const = 0;

    //! Get number of elements in buffer.
    virtual size_t size() const = 0;

    //! Set number of elements in buffer.
    //! @remarks
    //!  @p size should be less than or equal to max_size().
    //! @note
    //!  No constructor or destructor is called for buffer elements.
    virtual void set_size(size_t size) = 0;

    //! Check buffer integrity.
    //! @remarks
    //!  Calls roc_panic() if buffer is corrupted.
    virtual void check() const = 0;

    //! Print buffer to stdout.
    virtual void print() const = 0;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_IBUFFER_H_
