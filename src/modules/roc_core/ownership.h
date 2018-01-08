/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ownership.h
//! @brief Ownership policies.

#ifndef ROC_CORE_OWNERSHIP_H_
#define ROC_CORE_OWNERSHIP_H_

namespace roc {
namespace core {

template <class T, template <class TT> class Ownership> class SharedPtr;

//! Reference countable object ownership.
template <class T> struct RefCntOwnership {
    //! Pointer type returned from intrusive containers.
    //! @remarks
    //!  Container should return smart pointers instead of raw pointers since
    //!  it can call decref() on returned object later.
    typedef SharedPtr<T, core::RefCntOwnership> Pointer;

    //! Acquire ownership.
    static void acquire(T& object) {
        object.incref();
    }

    //! Release ownership.
    static void release(T& object) {
        object.decref();
    }
};

//! No ownership.
template <class T> struct NoOwnership {
    //! Pointer type returned from intrusive containers.
    //! @remarks
    //!  It's safe to return raw pointer since container will never free objects.
    typedef T* Pointer;

    //! Acquire ownership.
    static void acquire(T&) {
    }

    //! Release ownership.
    static void release(T&) {
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_OWNERSHIP_H_
