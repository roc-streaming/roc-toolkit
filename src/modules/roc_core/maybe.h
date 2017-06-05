/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/maybe.h
//! @brief Maybe.

#ifndef ROC_CORE_MAYBE_H_
#define ROC_CORE_MAYBE_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/attributes.h"
#include "roc_core/helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Maybe.
//!
//! Allows constructor call be delayed and optional for object
//! allocated on stack.
template <class T> class Maybe : public NonCopyable<> {
public:
    //! Storage constructor.
    //! @remarks
    //!  Doesn't call T constructor.
    Maybe()
        : allocated_(false) {
    }

    //! Storage destructor.
    //! @remarks
    //!  Calls T destructor if allocate() was called and release() wasn't.
    ~Maybe() {
        if (allocated_) {
            storage_.ref().~T();
        }
    }

    //! Return memory for object.
    //! @remarks
    //!  T constructor should be called by user on returned memory.
    //! @pre
    //!  Can be called only once until release() is called.
    void* allocate() {
        if (allocated_) {
            roc_panic("attempting to allocate `maybe' object twice");
        }
        allocated_ = true;
        return storage_.mem();
    }

    //! Forget that object was allocated.
    //! @remarks
    //!  T destructor will not be called.
    //! @pre
    //!  Can be called only once after every allocate() call.
    void release() {
        allocated_ = false;
    }

    //! Return allocated object pointer.
    //! @pre
    //!  Can be called only if allocate() was already called and release() wan't.
    const T* get() const {
        if (allocated_) {
            return &storage_.ref();
        } else {
            return NULL;
        }
    }

    //! Return allocated object pointer.
    //! @pre
    //!  Can be called only if allocate() was already called and release() wan't.
    T* get() {
        if (allocated_) {
            return &storage_.ref();
        } else {
            return NULL;
        }
    }

    //! Get pointer to raw memory.
    void* memory() {
        return storage_.mem();
    }

    //! Return allocated object pointer.
    //! @pre
    //!  Can be called only if allocate() was already called and release() wan't.
    const T* operator->() const {
        return safe_get_();
    }

    //! Return allocated object pointer.
    //! @pre
    //!  Can be called only if allocate() was already called and release() wan't.
    T* operator->() {
        return safe_get_();
    }

    //! Return allocated object reference.
    //! @pre
    //!  Can be called only if allocate() was already called and release() wan't.
    const T& operator*() const {
        return *safe_get_();
    }

    //! Return allocated object reference.
    //! @pre
    //!  Can be called only if allocate() was already called and release() wan't.
    T& operator*() {
        return *safe_get_();
    }

    //! Convert to bool.
    //! @returns
    //!  true if allocate() was called and release() wasn't.
    operator const struct unspecified_bool*() const {
        return (const unspecified_bool*)allocated_;
    }

    //! Get container.
    static Maybe& container_of(T& obj) {
        return *ROC_CONTAINER_OF(&Storage::container_of(obj), Maybe, storage_);
    }

private:
    T* safe_get_() {
        if (!allocated_) {
            roc_panic("attempting access non-allocated `maybe' object");
        }
        return &storage_.ref();
    }

    typedef AlignedStorage<T> Storage;

    Storage storage_;
    bool allocated_;
};

} // namespace core
} // namespace roc

//! Placement new for core::Maybe<T>.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
template <class T>
inline void* operator new(size_t size, roc::core::Maybe<T>& maybe) ROC_ATTR_NOTHROW {
    using namespace roc;
    roc_panic_if(size != sizeof(T));
    return maybe.allocate();
}

//! Placement delete for core::Maybe<T>.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <class T>
inline void operator delete(void* ptr, roc::core::Maybe<T>& maybe)ROC_ATTR_NOTHROW {
    using namespace roc;
    roc_panic_if(ptr != maybe.memory());
    maybe.release();
}

#endif // ROC_CORE_MAYBE_H_
