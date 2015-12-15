/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ibuffer_composer.h
//! @brief Buffer composer interface.

#ifndef ROC_CORE_IBUFFER_COMPOSER_H_
#define ROC_CORE_IBUFFER_COMPOSER_H_

#include "roc_core/ibuffer.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

//! Buffer composer interface.
//! @tparam T specifies element type.
template <class T> class IBufferComposer {
private:
    typedef SharedPtr<IBuffer<T> > IBufferPtr;

public:
    virtual ~IBufferComposer() {
    }

    //! Create buffer.
    //! @returns
    //!  NULL if buffer can't be created.
    virtual IBufferPtr compose() = 0;

    //! Restore existing buffer by pointer to its data.
    //! Example:
    //! @code
    //!  IBufferPtr buffer = composer.compose();
    //!  roc_panic_if_not(buffer == composer.container_of(buffer->data()));
    //! @endcode
    virtual IBufferPtr container_of(T* data) = 0;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_IBUFFER_COMPOSER_H_
