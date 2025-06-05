/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ref_counted_impl.h
//! @brief Implementation class for reference counter.

#ifndef ROC_CORE_REF_COUNTED_IMPL_H_
#define ROC_CORE_REF_COUNTED_IMPL_H_

#include "roc_core/atomic_int.h"

namespace roc {
namespace core {

//! Implementation class for reference counter.
//!
//! Allows to increment and decrement reference counter.
class RefCountedImpl {
public:
    //! Initialize.
    RefCountedImpl();

    ~RefCountedImpl();

    //! Get reference counter.
    int getref() const;

    //! Increment reference counter.
    //! @returns reference counter value after incrementing.
    int incref() const;

    //! Decrement reference counter.
    //! @returns reference counter value after decrementing.
    int decref() const;

private:
    enum { MaxCounter = 100000 };

    mutable AtomicInt<int32_t> counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REF_COUNTED_IMPL_H_
