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
    static size_t max_alignment();

    //! Return size aligned to maximum alignment.
    static size_t align_max(size_t size);

    //! Return size aligned to given alignment.
    static size_t align_as(size_t size, size_t alignment);

    //! Return padding needed for maximum alignment.
    static size_t pad_max(size_t size);

    //! Return padding needed for given alignment.
    static size_t pad_as(size_t size, size_t alignment);
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALIGN_OPS_H_
