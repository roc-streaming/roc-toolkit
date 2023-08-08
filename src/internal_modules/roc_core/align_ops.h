/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/align_ops.h
//! @brief Alignment operations.

#ifndef ROC_CORE_ALIGN_OPS_H_
#define ROC_CORE_ALIGN_OPS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Maximum aligned data unit.
union AlignMax {
    uint64_t u;  //!< 8-byte integer.
    double d;    //!< 8-byte floating point.
    void (*p)(); //!< 4-, 8- or 16-byte function pointer.
};

//! Alignment operations.
class AlignOps {
public:
    //! Get maximum alignment for current platform.
    static inline size_t max_alignment() {
        return sizeof(AlignMax);
    }

    //! Return size aligned to maximum alignment.
    static inline size_t align_max(size_t size) {
        return align_as(size, max_alignment());
    }

    //! Return size aligned to given alignment.
    static inline size_t align_as(size_t size, size_t alignment) {
        if (alignment == 0) {
            return size;
        }

        if (size % alignment != 0) {
            size += alignment - size % alignment;
        }

        return size;
    }

    //! Return padding needed for maximum alignment.
    static inline size_t pad_max(size_t size) {
        return pad_as(size, max_alignment());
    }

    //! Return padding needed for given alignment.
    static inline size_t pad_as(size_t size, size_t alignment) {
        if (alignment == 0) {
            return 0;
        }

        size_t new_size = size / alignment * alignment;
        if (new_size < size) {
            new_size += alignment;
        }

        return (new_size - size);
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALIGN_OPS_H_
