/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_H_
#define ROC_PIPELINE_TEST_HELPERS_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/sample_buffer.h"
#include "roc_core/heap_pool.h"
#include "roc_datagram/idatagram.h"
#include "roc_packet/ipacket.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace test {

template <size_t BufSz> inline audio::ISampleBufferPtr new_sample_buffer() {
    audio::ISampleBufferPtr buf =
        audio::SampleBufferTraits::default_composer<BufSz>().compose();

    CHECK(buf);

    buf->set_size(BufSz);

    for (size_t n = 0; n < buf->size(); n++) {
        buf->data()[n] = 9999;
    }

    return buf;
}

template <size_t BufSz> inline core::IByteBufferPtr new_byte_buffer() {
    core::IByteBufferPtr buf =
        core::ByteBufferTraits::default_composer<BufSz>().compose();

    CHECK(buf);

    buf->set_size(BufSz);

    for (size_t n = 0; n < buf->size(); n++) {
        buf->data()[n] = 0xff;
    }

    return buf;
}

inline datagram::Address new_address(datagram::port_t port) {
    datagram::Address addr;
    addr.ip[0] = 127;
    addr.ip[1] = 0;
    addr.ip[2] = 0;
    addr.ip[3] = 1;
    addr.port = port;
    return addr;
}

inline packet::IPacketPtr new_packet() {
    static rtp::Composer composer;

    packet::IPacketPtr packet = composer.compose(packet::IPacket::HasAudio);
    CHECK(packet);
    CHECK(packet->audio());

    return packet;
}

inline packet::IPacketConstPtr parse_packet(const datagram::IDatagram& dgm) {
    static rtp::Parser parser;

    packet::IPacketConstPtr packet = parser.parse(dgm.buffer());
    CHECK(packet);
    CHECK(packet->audio());

    return packet;
}

} // namespace test
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_H_
