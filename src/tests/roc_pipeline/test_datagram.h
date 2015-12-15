/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_DATAGRAM_H_
#define ROC_PIPELINE_TEST_DATAGRAM_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/noncopyable.h"

#include "roc_datagram/idatagram.h"
#include "roc_datagram/idatagram_composer.h"

namespace roc {
namespace test {

class TestDatagram : public datagram::IDatagram, public core::NonCopyable<> {
public:
    virtual datagram::DatagramType type() const {
        return "testDatagram";
    }

    virtual const core::IByteBufferConstSlice& buffer() const {
        return buffer_;
    }

    virtual void set_buffer(const core::IByteBufferConstSlice& buff) {
        buffer_ = buff;
    }

    virtual const datagram::Address& sender() const {
        return sender_;
    }

    virtual void set_sender(const datagram::Address& address) {
        sender_ = address;
    }

    virtual const datagram::Address& receiver() const {
        return receiver_;
    }

    virtual void set_receiver(const datagram::Address& address) {
        receiver_ = address;
    }

private:
    virtual void free() {
        delete this;
    }

    core::IByteBufferConstSlice buffer_;

    datagram::Address sender_;
    datagram::Address receiver_;
};

class TestDatagramComposer : public datagram::IDatagramComposer,
                             public core::NonCopyable<> {
public:
    virtual datagram::IDatagramPtr compose() {
        return new TestDatagram;
    }
};

} // namespace test
} // namespace roc

#endif // ROC_PIPELINE_TEST_DATAGRAM_H_
