/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/scoped_ptr.h
//! @brief Lexical scoped ownership pointer.

#ifndef ROC_CORE_SCOPED_PTR_H_
#define ROC_CORE_SCOPED_PTR_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Lexical scoped ownership pointer.
//!
//! @tparam T defines pointee type.
//! Pointee type should inherit one of the allocation policies (ArenaAllocation,
//! PoolAllocation, etc).
//!
//! ScopedPtr holds a pointer to an object and calls its dispose() method
//! (implemented by allocation policy) in destructor.
template <class T> class ScopedPtr : public NonCopyable<> {
public:
    //! Initialize.
    ScopedPtr(T* ptr = NULL)
        : ptr_(ptr) {
    }

    //! Destroy object.
    ~ScopedPtr() {
        reset();
    }

    //! Reset pointer to null.
    void reset(T* new_ptr = NULL) {
        if (ptr_ != NULL && ptr_ != new_ptr) {
            ptr_->dispose();
            ptr_ = NULL;
        }
        ptr_ = new_ptr;
    }

    //! Get underlying pointer and pass ownership to the caller.
    T* hijack() {
        T* ret = ptr_;
        if (ret == NULL) {
            roc_panic("scoped ptr: attempting to release a null pointer");
        }

        ptr_ = NULL;
        return ret;
    }

    //! Get underlying pointer.
    T* get() const {
        return ptr_;
    }

    //! Get underlying pointer.
    T* operator->() const {
        return ptr_;
    }

    //! Get underlying reference.
    T& operator*() const {
        if (ptr_ == NULL) {
            roc_panic("scoped ptr: attempting to dereference a null pointer");
        }
        return *ptr_;
    }

    //! Convert to bool.
    operator const struct unspecified_bool *() const {
        return (unspecified_bool*)ptr_;
    }

private:
    T* ptr_;
};

//! Equality check.
template <class T1, class T2, class P>
inline bool operator==(const ScopedPtr<T1>& a, const ScopedPtr<T2>& b) {
    return a.get() == b.get();
}

//! Equality check.
template <class T1, class T2, class P>
inline bool operator!=(const ScopedPtr<T1>& a, const ScopedPtr<T2>& b) {
    return a.get() != b.get();
}

} // namespace core
} // namespace roc

#endif // ROC_CORE_SCOPED_PTR_H_
