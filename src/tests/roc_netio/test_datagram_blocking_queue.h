/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_NETIO_TEST_DATAGRAM_BLOCKING_QUEUE_H_
#define ROC_NETIO_TEST_DATAGRAM_BLOCKING_QUEUE_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/noncopyable.h"
#include "roc_core/semaphore.h"

#include "roc_datagram/datagram_queue.h"
#include "roc_datagram/idatagram_reader.h"
#include "roc_datagram/idatagram_writer.h"

namespace roc {
namespace test {

class DatagramBlockingQueue : public datagram::IDatagramWriter,
                              public core::NonCopyable<> {
public:
    DatagramBlockingQueue()
        : queue_(0) {
    }

    ~DatagramBlockingQueue() {
        LONGS_EQUAL(0, queue_.size());
    }

    virtual void write(const datagram::IDatagramPtr& dgm) {
        queue_.write(dgm);
        sem_.post();
    }

    virtual datagram::IDatagramPtr read() {
        sem_.pend();
        return queue_.read();
    }

private:
    core::Semaphore sem_;
    datagram::DatagramQueue queue_;
};

} // namespace test
} // namespace roc

#endif // ROC_NETIO_TEST_DATAGRAM_BLOCKING_QUEUE_H_
