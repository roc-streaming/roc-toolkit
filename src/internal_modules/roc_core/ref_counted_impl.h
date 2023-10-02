/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ref_counted_impl.h
//! @brief Implementation class for object with reference counter.

#ifndef ROC_CORE_REF_COUNTED_IMPL_H_
#define ROC_CORE_REF_COUNTED_IMPL_H_

#include "roc_core/atomic.h"

namespace roc {
namespace core {

class RefCountedImpl {
public:
    //! Initialize.
    RefCountedImpl();

    ~RefCountedImpl();

    //! Get reference counter.
    int getref() const;

    //! Increment reference counter.
    //! @returns counter value after incrementing.
    int incref() const;

    //! Decrement reference counter.
    //! @returns counter value after decrementing.
    int decref() const;

private:
    enum { MaxCounter = 100000 };

    mutable Atomic<int> counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REF_COUNTED_IMPL_H_
