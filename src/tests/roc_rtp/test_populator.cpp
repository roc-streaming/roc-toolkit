/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_format.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/populator.h"
#include "roc_status/status_code.h"
#include "test_helpers/status_reader.h"

namespace roc {
namespace rtp {

namespace {

enum { ChMask = 3, PacketSz = 128, SampleRate = 10000 };

const audio::SampleSpec SampleSpec = audio::SampleSpec(SampleRate,
                                                       audio::Sample_RawFormat,
                                                       audio::ChanLayout_Surround,
                                                       audio::ChanOrder_Smpte,
                                                       ChMask);

const audio::PcmFormat PcmFmt = audio::PcmFormat_SInt16_Be;

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> buffer_factory(arena, PacketSz);

packet::PacketPtr new_packet(packet::stream_timestamp_t duration) {
    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(packet::Packet::FlagRTP);
    packet->rtp()->payload_type = PayloadType_L16_Stereo;
    packet->rtp()->duration = duration;

    core::Slice<uint8_t> buffer = buffer_factory.new_buffer();
    CHECK(buffer);
    packet->rtp()->payload = buffer;

    return packet;
}

} // namespace

TEST_GROUP(populator) {};

TEST(populator, failed_to_read_packet) {
    const status::StatusCode codes[] = {
        status::StatusUnknown,
        status::StatusNoData,
    };

    for (unsigned n = 0; n < ROC_ARRAY_SIZE(codes); ++n) {
        test::StatusReader reader(codes[n]);
        audio::PcmDecoder decoder(PcmFmt, SampleSpec);
        Populator populator(reader, decoder, SampleSpec);

        packet::PacketPtr pp;
        LONGS_EQUAL(codes[n], populator.read(pp));
        CHECK(!pp);
    }
}

TEST(populator, empty_duration) {
    packet::Queue queue;
    audio::PcmDecoder decoder(PcmFmt, SampleSpec);
    Populator populator(queue, decoder, SampleSpec);

    const packet::stream_timestamp_t packet_duration = 0;
    const packet::stream_timestamp_t expected_duration = 32;

    packet::PacketPtr wp = new_packet(packet_duration);
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.write(wp));

    packet::PacketPtr rp;
    LONGS_EQUAL(0, populator.read(rp));
    CHECK(rp);
    CHECK(wp == rp);

    LONGS_EQUAL(expected_duration, rp->rtp()->duration);
}

TEST(populator, non_empty_duration) {
    packet::Queue queue;
    audio::PcmDecoder decoder(PcmFmt, SampleSpec);
    Populator populator(queue, decoder, SampleSpec);

    const packet::stream_timestamp_t duration = 100;

    core::Slice<uint8_t> buffer = buffer_factory.new_buffer();
    CHECK(buffer);
    packet::PacketPtr wp = new_packet(duration);
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.write(wp));

    packet::PacketPtr rp;
    LONGS_EQUAL(0, populator.read(rp));
    CHECK(rp);
    CHECK(wp == rp);
    LONGS_EQUAL(duration, rp->rtp()->duration);
}

} // namespace rtp
} // namespace roc
