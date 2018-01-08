/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/refcnt.h
//! @brief Base class for reference countable objects.

#ifndef ROC_CORE_REFCNT_H_
#define ROC_CORE_REFCNT_H_

#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Base class for reference countable objects.
//!
//! @tparam T defines the derived class, which should provide free() method.
template <class T> class RefCnt : public NonCopyable<RefCnt<T> > {
public:
    RefCnt()
        : counter_(0) {
    }

    ~RefCnt() {
        if (counter_ != 0) {
            roc_panic("refcnt: reference counter is non-zero in destructor, counter=%d",
                      (int)counter_);
        }
    }

    //! Get reference counter.
    long getref() const {
        return counter_;
    }

    //! Increment reference counter.
    void incref() const {
        if (counter_ < 0) {
            roc_panic("refcnt: attempting to call incref() on freed object");
        }
        ++counter_;
    }

    //! Decrement reference counter.
    //! @remarks
    //!  Calls free() if reference counter becomes zero.
    void decref() const {
        if (counter_ <= 0) {
            roc_panic("refcnt: attempting to call decref() on destroyed object");
        }
        if (--counter_ == 0) {
            static_cast<T*>(const_cast<RefCnt*>(this))->destroy();
        }
    }

private:
    mutable Atomic counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REFCNT_H_
