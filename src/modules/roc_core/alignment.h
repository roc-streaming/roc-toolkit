/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/alignment.h
//! @brief Alignment.

#ifndef ROC_CORE_ALIGNMENT_H_
#define ROC_CORE_ALIGNMENT_H_

#include "roc_core/macros.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Calculate padding required for given alignment.
inline size_t padding(size_t size, size_t alignment) {
    if (alignment == 0) {
        return 0;
    }
    size_t new_size = size / alignment * alignment;
    if (new_size < size) {
        new_size += alignment;
    }
    return (new_size - size);
}

//! Adjust the given size to be maximum aligned.
inline size_t max_align(size_t sz) {
    enum { Align = ROC_MAX(sizeof(void (*)()), sizeof(double)) };
    if (sz % Align != 0) {
        sz += Align - sz % Align;
    }
    return sz;
}

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALIGNMENT_H_
