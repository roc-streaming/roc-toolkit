/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
class RefCnt : public NonCopyable<RefCnt> {
public:
    RefCnt();

    //! Enable memory leak detection.
    //! @remarks
    //!  When enabled, a global object's destructor will abort() if there
    //!  are RefCnt objects not destroyed yet.
    static void enable_leak_detection();

    //! Get reference counter.
    long getref() const {
        return counter_;
    }

    //! Increment reference counter.
    void incref() const {
        if (counter_ < 0) {
            roc_panic("attempting to call incref() on freed object");
        }
        ++counter_;
    }

    //! Decrement reference counter.
    //! @remarks
    //!  Calls free() if reference counter becomes zero.
    void decref() const {
        if (counter_ <= 0) {
            roc_panic("attempting to call decref() on freed object");
        }

        if (--counter_ == 0) {
            const_cast<RefCnt&>(*this).free();
        }
    }

protected:
    virtual ~RefCnt();

private:
    //! Free object.
    //! @remarks
    //!  Invoked when reference counter becomes zero.
    virtual void free() = 0;

    mutable Atomic counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REFCNT_H_
