/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/stream.h"

namespace roc {
namespace netio {

Stream::~Stream() {
    while (StreamBufferPtr bp = buffers_.front()) {
        buffers_.remove(*bp);
    }
}

size_t Stream::size() const {
    core::Mutex::Lock lock(mutex_);

    size_t ret = 0;

    for (StreamBufferPtr bp = buffers_.front(); bp; bp = buffers_.nextof(*bp)) {
        ret += bp->size();
    }

    return ret;
}

void Stream::append(const StreamBufferPtr& buffer) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(buffer);

    buffers_.push_back(*buffer);
}

ssize_t Stream::read(char* buf, size_t len) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(buf);

    if (len < 1) {
        return -1;
    }

    size_t off = 0;

    StreamBufferPtr curr = buffers_.front();
    while (curr) {
        StreamBufferPtr next = buffers_.nextof(*curr);

        const ssize_t bytes = curr->read(buf + off, len);
        if (bytes == -1) {
            break;
        }

        if (!curr->size()) {
            buffers_.remove(*curr);
        }

        len -= (size_t)bytes;
        off += (size_t)bytes;

        curr = next;
    }

    return (ssize_t)off;
}

} // namespace netio
} // namespace roc
