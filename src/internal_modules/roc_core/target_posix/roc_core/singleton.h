/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/singleton.h
//! @brief Singleton.

#ifndef ROC_CORE_SINGLETON_H_
#define ROC_CORE_SINGLETON_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

#include <pthread.h>

namespace roc {
namespace core {

//! Singleton.
template <class T> class Singleton : public NonCopyable<> {
public:
    //! Get singleton instance.
    static T& instance() {
        T* inst = AtomicOps::load_relaxed(instance_);

        if (!inst) {
            pthread_once(&once_, create_);
            inst = AtomicOps::load_relaxed(instance_);
        }

        roc_panic_if_not(inst);
        return *inst;
    }

private:
    static void create_() {
        T* inst = new (storage_.memory()) T;
        AtomicOps::store_release(instance_, inst);
    }

    static pthread_once_t once_;
    static AlignedStorage<sizeof(T)> storage_;
    static T* instance_;
};

template <class T> pthread_once_t Singleton<T>::once_ = PTHREAD_ONCE_INIT;
template <class T> AlignedStorage<sizeof(T)> Singleton<T>::storage_;
template <class T> T* Singleton<T>::instance_;

} // namespace core
} // namespace roc

#endif // ROC_CORE_SINGLETON_H_
