/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/buffer.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

Buffer::Buffer(IPool& buffer_pool, size_t buffer_size)
    : RefCounted<Buffer, PoolAllocation>(buffer_pool)
    , size_(buffer_size) {
    roc_panic_if_msg(sizeof(Buffer) + buffer_size != buffer_pool.object_size(),
                     "buffer: attempt to create buffer with wrong size:"
                     " requested=%lu expected=%lu",
                     (unsigned long)sizeof(Buffer) + buffer_size,
                     (unsigned long)buffer_pool.object_size());

    new (data_) uint8_t[size_];
}

} // namespace core
} // namespace roc
