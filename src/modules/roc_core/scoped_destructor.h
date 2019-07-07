/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/scoped_destructor.h
//! @brief Scoped destructor.

#ifndef ROC_CORE_SCOPED_DESTRUCTOR_H_
#define ROC_CORE_SCOPED_DESTRUCTOR_H_

#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Destroys the object via custom deleter.
template <class T, void (*Func)(T)> class ScopedDestructor : public NonCopyable<> {
public:
    //! Initialize.
    explicit ScopedDestructor(T obj)
        : obj_(obj) {
    }

    //! Destroy.
    ~ScopedDestructor() {
        Func(obj_);
    }

private:
    T obj_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SCOPED_DESTRUCTOR_H_
