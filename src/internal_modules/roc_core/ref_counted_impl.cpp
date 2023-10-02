/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ref_counted_impl.cpp
//! @brief Implementation class for object with reference counter.

#include "roc_core/ref_counted_impl.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

RefCountedImpl::RefCountedImpl()
    : counter_(0) {
}

RefCountedImpl::~RefCountedImpl() {
    if (!counter_.compare_exchange(0, -1)) {
        roc_panic("ref counter:"
                  " attempt to destroy object that is in use, destroyed, or corrupted:"
                  " counter=%d",
                  (int)counter_);
    }
}

int RefCountedImpl::getref() const {
    const int current_counter = counter_;

    if (current_counter < 0 || current_counter > MaxCounter) {
        roc_panic("ref counter:"
                  " attempt to access destroyed or currupted object:"
                  " counter=%d",
                  (int)current_counter);
    }

    return current_counter;
}

int RefCountedImpl::incref() const {
    const int current_counter = ++counter_;

    if (current_counter < 0 || current_counter > MaxCounter) {
        roc_panic("ref counter:"
                  " attempt to access destroyed or currupted object"
                  " counter=%d",
                  (int)current_counter);
    }

    return current_counter;
}

int RefCountedImpl::decref() const {
    const int current_counter = --counter_;

    if (current_counter < 0 || current_counter > MaxCounter) {
        roc_panic("ref counter:"
                  " attempt to access destroyed or currupted object"
                  " counter=%d",
                  (int)current_counter);
    }
    return current_counter;
}

} // namespace core
} // namespace roc
