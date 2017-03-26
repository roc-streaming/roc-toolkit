/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/scoped_ptr.h
//! @brief Scoped pointer.

#ifndef ROC_CORE_SCOPED_PTR_H_
#define ROC_CORE_SCOPED_PTR_H_

#include "roc_core/stddefs.h"
#include "roc_core/panic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership.h"

namespace roc {
namespace core {

//! Scoped pointer.
//!
//! @tparam T defines pointee type. It may be const.
//! @tparam Ownership defines methods to acquire and release ownership of object.
//! If NewOwnership is used, delete will be called to free object.
template <class T, template <class TT> class Ownership = NewOwnership>
class ScopedPtr : public NonCopyable<> {
public:
    //! Initialize from raw pointer.
    explicit ScopedPtr(T* ptr = NULL)
        : ptr_(ptr) {
        acquire_();
    }

    //! Destroy object.
    ~ScopedPtr() {
        release_();
    }

    //! Reset pointer to new value.
    void reset(T* new_ptr = NULL) {
        if (new_ptr != ptr_) {
            release_();
            ptr_ = new_ptr;
        }
    }

    //! Get underlying pointer and pass ownership to the caller.
    T* release() {
        T* ret = ptr_;
        if (ret == NULL) {
            roc_panic("attempting to release null scoped pointer");
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
            roc_panic("attempting to dereference null scoped pointer");
        }
        return *ptr_;
    }

    //! Convert to bool.
    operator const struct unspecified_bool*() const {
        return (unspecified_bool*)ptr_;
    }

private:
    void acquire_() {
        if (ptr_ != NULL) {
            Ownership<T>::acquire(*ptr_);
        }
    }

    void release_() {
        if (ptr_ != NULL) {
            Ownership<T>::release(*ptr_);
        }
    }

private:
    T* ptr_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SCOPED_PTR_H_
