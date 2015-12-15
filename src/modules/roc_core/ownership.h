/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ownership.h
//! @brief Ownership holders.

#ifndef ROC_CORE_OWNERSHIP_H_
#define ROC_CORE_OWNERSHIP_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! No ownership.
template <class T> struct NoOwnership {
    //! Pointer type returned from intrusive containers.
    //! @remarks
    //!  It's safe to return raw pointer since container will never free objects.
    typedef T* SafePtr;

    //! Acquire ownership.
    static void acquire(T&) {
    }

    //! Release ownership.
    static void release(T&) {
    }
};

template <class T, template <class TT> class Ownership> class SharedPtr;

//! Reference countable object ownership.
template <class T> struct RefCntOwnership {
    //! Pointer type returned from intrusive containers.
    //! @remarks
    //!  Container should return smart pointers instead of raw pointers since
    //!  it can call decref() on returned object later.
    typedef SharedPtr<T, core::RefCntOwnership> SafePtr;

    //! Acquire ownership.
    static void acquire(T& obj) {
        obj.incref();
    }

    //! Release ownership.
    static void release(T& obj) {
        obj.decref();
    }
};

//! Unique ownership of object allocated using new.
template <class T> struct NewOwnership {
    //! Pointer type returned from intrusive containers.
    //! @remarks
    //!  Not defined, since container can destroy objects and it's not safe
    //!  to return pointers to them.
    typedef void SafePtr;

    //! Acquire ownership.
    static void acquire(T&) {
    }

    //! Release ownership.
    static void release(T& obj) {
        delete &obj;
    }
};

//! Unique ownership of object allocated using malloc().
template <class T> struct MallocOwnership {
    //! Pointer type returned from intrusive containers.
    //! @remarks
    //!  Not defined, since container can destroy objects and it's not safe
    //!  to return pointers to them.
    typedef void SafePtr;

    //! Acquire ownership.
    static void acquire(T&) {
    }

    //! Release ownership.
    static void release(T& obj) {
        free(&obj);
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_OWNERSHIP_H_
