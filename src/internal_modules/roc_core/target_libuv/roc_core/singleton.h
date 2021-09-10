/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libuv/roc_core/singleton.h
//! @brief Singleton.

#ifndef ROC_CORE_SINGLETON_H_
#define ROC_CORE_SINGLETON_H_

#include <uv.h>

#include "roc_core/alignment.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Singleton.
template <class T> class Singleton : public core::NonCopyable<> {
public:
    //! Get singleton instance.
    static T& instance() {
        T* inst = AtomicOps::load_relaxed(instance_);
        if (!inst) {
            uv_once(&once_, create_);
            inst = AtomicOps::load_relaxed(instance_);
        }
        roc_panic_if_not(inst);
        return *inst;
    }

private:
    union Storage {
        MaxAlign align;
        char mem[sizeof(T)];
    };

    static void create_() {
        T* inst = new (storage_.mem) T();
        AtomicOps::store_release(instance_, inst);
    }

    static uv_once_t once_;
    static Storage storage_;
    static T* instance_;
};

template <class T> uv_once_t Singleton<T>::once_ = UV_ONCE_INIT;
template <class T> typename Singleton<T>::Storage Singleton<T>::storage_;
template <class T> T* Singleton<T>::instance_;

} // namespace core
} // namespace roc

#endif // ROC_CORE_SINGLETON_H_
