/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/singleton.h
//! @brief Singleton object.

#ifndef ROC_CORE_SINGLETON_H_
#define ROC_CORE_SINGLETON_H_

#include "roc_core/maybe.h"
#include "roc_core/noncopyable.h"
#include "roc_core/spin_mutex.h"

namespace roc {
namespace core {

//! Singleton.
template <class T> class Singleton : public NonCopyable<> {
public:
    //! Get lazy-constructed instance.
    //! @note
    //!  Implementation doesn't support calling before main(), i.e. before
    //!  static member are constructed.
    static T& instance() {
        SpinMutex::Lock lock(mutex_);
        if (!instance_) {
            new (instance_) T();
        }
        return *instance_;
    }

private:
    static SpinMutex mutex_;
    static Maybe<T> instance_;
};

template <class T> SpinMutex Singleton<T>::mutex_;
template <class T> Maybe<T> Singleton<T>::instance_;

} // namespace core
} // namespace roc

#endif // ROC_CORE_SINGLETON_H_
