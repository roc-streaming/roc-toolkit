/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/scoped_release.h
//! @brief Lexical scoped releaser.

#ifndef ROC_CORE_SCOPED_RELEASE_H_
#define ROC_CORE_SCOPED_RELEASE_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Lexical scoped releaser.
//!
//! @tparam T defines pointee type.
//!
//! ScopedRelease holds a pointer to an object and calls custom releaser
//! function for the object in destructor.
template <class T> class ScopedRelease : public NonCopyable<> {
public:
    //! Initialize.
    template <class Arg>
    ScopedRelease(T* ptr, void (*release_func)(Arg*))
        : ptr_(ptr)
        , del_fn_((ReleaseFunc)release_func) {
        if (!del_fn_) {
            roc_panic("scoped release: null function");
        }
    }

    //! Destroy object.
    ~ScopedRelease() {
        reset();
    }

    //! Reset pointer to a new value.
    void reset(T* new_ptr = NULL) {
        if (ptr_ != NULL && ptr_ != new_ptr) {
            del_fn_(ptr_);
            ptr_ = NULL;
        }
        ptr_ = new_ptr;
    }

    //! Get underlying pointer and pass ownership to the caller.
    T* hijack() {
        T* ret = ptr_;
        if (ret == NULL) {
            roc_panic("scoped release: attempt to release a null pointer");
        }

        ptr_ = NULL;
        return ret;
    }

private:
    typedef void (*ReleaseFunc)(void*);

    T* ptr_;
    ReleaseFunc del_fn_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SCOPED_RELEASE_H_
