/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/optional.h
//! @brief Optionally constructed object.

#ifndef ROC_CORE_OPTIONAL_H_
#define ROC_CORE_OPTIONAL_H_

#include "roc_core/alignment.h"
#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Optionally constructed object.
template <class T> class Optional : public NonCopyable<> {
public:
    Optional()
        : ptr_(NULL) {
    }

    ~Optional() {
        if (ptr_) {
            ptr_->~T();
        }
    }

    //! Set pointer to the newly created object, destroy old pointed object if set.
    //! @pre
    //!  @p ptr should be returned by associated placement new.
    void reset(T* ptr) {
        if (ptr_) {
            ptr_->~T();
        }
        if (ptr && (void*)ptr != storage_.mem) {
            roc_panic("optional: attempt to set incorrect object");
        }
        ptr_ = ptr;
    }

    //! Get underlying object.
    T* get() const {
        return ptr_;
    }

    //! Get underlying object.
    T* operator->() const {
        if (ptr_ == NULL) {
            roc_panic("optional: attempt to dereference unitialized object");
        }
        return ptr_;
    }

    //! Get underlying reference.
    T& operator*() const {
        if (ptr_ == NULL) {
            roc_panic("optional: attempt to dereference unitialized object");
        }
        return *ptr_;
    }

    //! Convert to bool.
    operator const struct unspecified_bool *() const {
        return (const unspecified_bool*)ptr_;
    }

    //! Get storage for the object.
    void* storage() {
        if (ptr_) {
            roc_panic("optional: attempt to get storage after the object was created");
        }
        return storage_.mem;
    }

private:
    union Storage {
        MaxAlign align;
        char mem[sizeof(T)];
    };

    T* ptr_;
    Storage storage_;
};

} // namespace core
} // namespace roc

//! Placement new for core::Optional.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
template <class T>
inline void* operator new(size_t, roc::core::Optional<T>& opt) throw() {
    return opt.storage();
}

//! Placement delete for core::Optional.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <class T> inline void operator delete(void*, roc::core::Optional<T>&) throw() {
}

#endif // ROC_CORE_OPTIONAL_H_
