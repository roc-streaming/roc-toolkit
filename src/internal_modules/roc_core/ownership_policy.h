/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ownership_policy.h
//! @brief Ownership policies.

#ifndef ROC_CORE_OWNERSHIP_POLICY_H_
#define ROC_CORE_OWNERSHIP_POLICY_H_

namespace roc {
namespace core {

template <class T, template <class TT> class Ownership> class SharedPtr;

//! Reference counted object ownership.
template <class T> struct RefCountedOwnership {
    //! Pointer type returned from containers.
    typedef SharedPtr<T, core::RefCountedOwnership> Pointer;

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
    //! Pointer type returned from containers.
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

#endif // ROC_CORE_OWNERSHIP_POLICY_H_
