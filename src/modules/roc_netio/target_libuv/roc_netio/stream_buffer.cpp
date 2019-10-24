/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/stream_buffer.h"

namespace roc {
namespace netio {

StreamBuffer::StreamBuffer(core::IAllocator& allocator)
    : allocator_(allocator)
    , data_(allocator)
    , offset_(0) {
}

size_t StreamBuffer::size() const {
    return data_.size() - offset_;
}

char* StreamBuffer::data() {
    if (!size()) {
        return NULL;
    }

    return &data_[0] + offset_;
}

bool StreamBuffer::resize(size_t new_size) {
    return data_.resize(new_size);
}

ssize_t StreamBuffer::read(char* buf, size_t len) {
    roc_panic_if_not(buf);

    if (len < 1) {
        return -1;
    }

    if (!size()) {
        return -1;
    }

    if (len > size()) {
        len = size();
    }

    memcpy(buf, data(), len);

    offset_ += len;

    return (ssize_t)len;
}

void StreamBuffer::destroy() {
    allocator_.destroy(*this);
}

} // namespace netio
} // namespace roc
