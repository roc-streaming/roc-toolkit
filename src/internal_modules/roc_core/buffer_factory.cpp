/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/buffer_factory.h"

namespace roc {
namespace core {

BufferFactory::BufferFactory(IArena& arena, size_t buffer_size)
    : buffer_pool_("buffer_pool", arena, sizeof(Buffer) + buffer_size)
    , buffer_size_(buffer_size) {
}

size_t BufferFactory::buffer_size() const {
    return buffer_size_;
}

BufferPtr BufferFactory::new_buffer() {
    return new (buffer_pool_) Buffer(buffer_pool_, buffer_size_);
}

} // namespace core
} // namespace roc
