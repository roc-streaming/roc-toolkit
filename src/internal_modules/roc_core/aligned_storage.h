/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/aligned_storage.h
//! @brief Aligned storage.

#ifndef ROC_CORE_ALIGNED_STORAGE_H_
#define ROC_CORE_ALIGNED_STORAGE_H_

#include "roc_core/align_ops.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Fixed-size maximum-aligned storage.
template <size_t Size> class AlignedStorage {
public:
    //! Get storage size.
    static size_t size() {
        return sizeof(Memory);
    }

    //! Get storage memory.
    const void* memory() const {
        return memory_.payload;
    }

    //! Get storage memory.
    void* memory() {
        return memory_.payload;
    }

private:
    union Memory {
        AlignMax alignment;
        char payload[Size != 0 ? Size : 1];
    };

    Memory memory_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALIGNED_STORAGE_H_
