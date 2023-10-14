/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/align_ops.h"

namespace roc {
namespace core {

size_t AlignOps::max_alignment() {
    return sizeof(AlignMax);
}

size_t AlignOps::align_max(size_t size) {
    return align_as(size, max_alignment());
}

size_t AlignOps::align_as(size_t size, size_t alignment) {
    if (alignment == 0) {
        return size;
    }

    if (size % alignment != 0) {
        size += alignment - size % alignment;
    }

    return size;
}

size_t AlignOps::pad_max(size_t size) {
    return pad_as(size, max_alignment());
}

size_t AlignOps::pad_as(size_t size, size_t alignment) {
    if (alignment == 0) {
        return 0;
    }

    size_t new_size = size / alignment * alignment;
    if (new_size < size) {
        new_size += alignment;
    }

    return (new_size - size);
}

} // namespace core
} // namespace roc
