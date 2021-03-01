/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/scoped_ptr.h
//! @brief Unique ownrship pointer.

#ifndef ROC_CORE_SCOPED_PTR_H_
#define ROC_CORE_SCOPED_PTR_H_

#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Unique ownrship pointer.
//!
//! @tparam T defines pointee type. It may be const.
//! @tparam Destroyer is used to destroy the object.
template <class T, class Destroyer = IAllocator> class ScopedPtr : public NonCopyable<> {
public:
    //! Initialize null pointer.
    ScopedPtr()
        : ptr_(NULL)
        , destroyer_() {
    }

    //! Initialize from a raw pointer.
    ScopedPtr(T* ptr, Destroyer& destroyer)
        : ptr_(ptr)
        , destroyer_(&destroyer) {
    }

    //! Destroy object.
    ~ScopedPtr() {
        destroy_();
    }

    //! Reset pointer to null.
    void reset() {
        destroy_();
        ptr_ = NULL;
        destroyer_ = NULL;
    }

    //! Reset pointer to a new value.
    void reset(T* new_ptr, Destroyer& new_destroyer) {
        if (new_ptr != ptr_) {
            destroy_();
            ptr_ = new_ptr;
            destroyer_ = &new_destroyer;
        }
    }

    //! Get underlying pointer and pass ownership to the caller.
    T* release() {
        T* ret = ptr_;
        if (ret == NULL) {
            roc_panic("uniqueptr: attempting to release a null pointer");
        }
        ptr_ = NULL;
        destroyer_ = NULL;
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
            roc_panic("unique ptr: attempting to dereference a null pointer");
        }
        return *ptr_;
    }

    //! Convert to bool.
    operator const struct unspecified_bool *() const {
        return (unspecified_bool*)ptr_;
    }

private:
    void destroy_() {
        if (ptr_) {
            roc_panic_if(destroyer_ == NULL);
            destroyer_->destroy(*ptr_);
        }
    }

    T* ptr_;
    Destroyer* destroyer_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SCOPED_PTR_H_
