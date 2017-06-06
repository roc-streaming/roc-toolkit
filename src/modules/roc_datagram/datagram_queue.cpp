/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_datagram/datagram_queue.h"
#include "roc_core/log.h"

namespace roc {
namespace datagram {

DatagramQueue::DatagramQueue(size_t max_size)
    : max_size_(max_size) {
}

IDatagramPtr DatagramQueue::read() {
    Lock lock(mutex_);

    IDatagramPtr dgm = list_.front();
    if (dgm) {
        list_.remove(*dgm);
    }

    return dgm;
}

void DatagramQueue::write(const IDatagramPtr& dgm) {
    Lock lock(mutex_);

    if (!dgm) {
        roc_panic("attempting to write null datagram to datagram queue");
    }

    if (max_size_ != 0 && list_.size() == max_size_) {
        roc_log(LogDebug, "datagram queue is full, dropping oldest datagram (size = %lu)",
                (unsigned long)max_size_);

        if (IDatagramPtr head = list_.front()) {
            list_.remove(*head);
        }
    }

    list_.append(*dgm);
}

size_t DatagramQueue::size() const {
    Lock lock(mutex_);

    return list_.size();
}

} // namespace datagram
} // namespace roc
