/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/buffer.h"

namespace roc {
namespace core {

Buffer::Buffer(IPool& buffer_pool, size_t buffer_size)
    : RefCounted<Buffer, PoolAllocation>(buffer_pool)
    , size_(buffer_size) {
    new (data()) uint8_t[size()];
}

} // namespace core
} // namespace roc
