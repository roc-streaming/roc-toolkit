/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_gnu/roc_core/aligned_storage.h
//! @brief Aligned storage.

#ifndef ROC_CORE_ALIGNED_STORAGE_H_
#define ROC_CORE_ALIGNED_STORAGE_H_

#include "roc_core/attributes.h"
#include "roc_core/helpers.h"

namespace roc {
namespace core {

//! Aligned storage.
template <class T> class AlignedStorage {
public:
    //! Get reference to const T.
    const T& ref() const {
        return *(const T*)(const void*)mem();
    }

    //! Get reference to T.
    T& ref() {
        return *(T*)(void*)mem();
    }

    //! Get pointer to raw memory.
    const unsigned char* mem() const {
        return mem_;
    }

    //! Get pointer to raw memory.
    unsigned char* mem() {
        return mem_;
    }

    //! Get container.
    static AlignedStorage& container_of(T& obj) {
        return *ROC_CONTAINER_OF(&obj, AlignedStorage, mem_);
    }

private:
    ROC_ALIGN_AS(T) unsigned char mem_[sizeof(T)];
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALIGNED_STORAGE_H_
