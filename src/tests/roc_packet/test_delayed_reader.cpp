/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_packet/delayed_reader.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

namespace {

enum { SampleRate = 1000, NumSamples = 100, NumPackets = 30, MaxBufSize = 100 };

const core::nanoseconds_t NsPerSample = core::Second / SampleRate;

const audio::SampleSpec sample_spec(SampleRate,
                                    audio::PcmSubformat_Raw,
                                    audio::ChanLayout_Surround,
                                    audio::ChanOrder_Smpte,
                                    audio::ChanMask_Surround_Stereo);

core::HeapArena arena;
PacketFactory packet_factory(arena, MaxBufSize);

PacketPtr new_packet(seqnum_t sn) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP);
    packet->rtp()->seqnum = sn;
    packet->rtp()->stream_timestamp = stream_timestamp_t(sn * NumSamples);
    packet->rtp()->duration = NumSamples;

    return packet;
}

void write_packet(IWriter& writer, const PacketPtr& pp) {
    CHECK(pp);
    LONGS_EQUAL(status::StatusOK, writer.write(pp));
}

PacketPtr
expect_read(status::StatusCode expect_code, IReader& reader, PacketReadMode mode) {
    PacketPtr pp;
    LONGS_EQUAL(expect_code, reader.read(pp, mode));
    if (expect_code == status::StatusOK) {
        CHECK(pp);
    } else {
        CHECK(!pp);
    }
    return pp;
}

class MockReader : public IReader {
public:
    explicit MockReader(status::StatusCode code)
        : code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr& pp,
                                                       PacketReadMode mode) {
        return code_;
    }

private:
    status::StatusCode code_;
};

} // namespace

TEST_GROUP(delayed_reader) {};

TEST(delayed_reader, no_delay) {
    FifoQueue queue;
    DelayedReader dr(queue, 0, sample_spec);
    LONGS_EQUAL(status::StatusOK, dr.init_status());

    PacketPtr pp = expect_read(status::StatusDrain, dr, ModeFetch);
    CHECK(!pp);

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr wp = new_packet(n);
        write_packet(queue, wp);

        PacketPtr rp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(wp == rp);
    }
}

TEST(delayed_reader, delay) {
    FifoQueue queue;
    DelayedReader dr(queue, NumSamples * NumPackets * NsPerSample, sample_spec);
    LONGS_EQUAL(status::StatusOK, dr.init_status());

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp = expect_read(status::StatusDrain, dr, ModeFetch);
        CHECK(!pp);

        packets[n] = new_packet(n);
        write_packet(queue, packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(pp == packets[n]);
    }

    PacketPtr pp = expect_read(status::StatusDrain, dr, ModeFetch);
    CHECK(!pp);

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr wp = new_packet(NumPackets + n);
        write_packet(queue, wp);

        PacketPtr rp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(wp == rp);
    }

    pp = expect_read(status::StatusDrain, dr, ModeFetch);
    CHECK(!pp);
}

TEST(delayed_reader, instant) {
    FifoQueue queue;
    DelayedReader dr(queue, NumSamples * NumPackets * NsPerSample, sample_spec);
    LONGS_EQUAL(status::StatusOK, dr.init_status());

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet(n);
        write_packet(queue, packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(pp == packets[n]);
    }

    PacketPtr pp = expect_read(status::StatusDrain, dr, ModeFetch);
    CHECK(!pp);
}

TEST(delayed_reader, trim) {
    FifoQueue queue;
    DelayedReader dr(queue, NumSamples * NumPackets * NsPerSample, sample_spec);
    LONGS_EQUAL(status::StatusOK, dr.init_status());

    PacketPtr packets[NumPackets * 2];

    for (seqnum_t n = 0; n < NumPackets * 2; n++) {
        packets[n] = new_packet(n);
        write_packet(queue, packets[n]);
    }

    for (seqnum_t n = NumPackets; n < NumPackets * 2; n++) {
        PacketPtr pp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(pp == packets[n]);
    }

    PacketPtr pp = expect_read(status::StatusDrain, dr, ModeFetch);
    CHECK(!pp);
}

TEST(delayed_reader, late_duplicates) {
    FifoQueue queue;
    DelayedReader dr(queue, NumSamples * NumPackets * NsPerSample, sample_spec);
    LONGS_EQUAL(status::StatusOK, dr.init_status());

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet(n);
        write_packet(queue, packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(pp == packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr wp = new_packet(n);
        write_packet(queue, wp);

        PacketPtr rp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(wp == rp);
    }

    PacketPtr pp = expect_read(status::StatusDrain, dr, ModeFetch);
    CHECK(!pp);
}

TEST(delayed_reader, fetch_peek) {
    FifoQueue queue;
    DelayedReader dr(queue, NumSamples * NumPackets * NsPerSample, sample_spec);
    LONGS_EQUAL(status::StatusOK, dr.init_status());

    PacketPtr packets[NumPackets * 2];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp;

        pp = expect_read(status::StatusDrain, dr, ModePeek);
        CHECK(!pp);

        pp = expect_read(status::StatusDrain, dr, ModeFetch);
        CHECK(!pp);

        packets[n] = new_packet(n);
        write_packet(queue, packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp;

        pp = expect_read(status::StatusOK, dr, ModePeek);
        CHECK(pp == packets[n]);

        pp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(pp == packets[n]);

        packets[NumPackets + n] = new_packet(NumPackets + n);
        write_packet(queue, packets[NumPackets + n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp;

        pp = expect_read(status::StatusOK, dr, ModePeek);
        CHECK(pp == packets[NumPackets + n]);

        pp = expect_read(status::StatusOK, dr, ModeFetch);
        CHECK(pp == packets[NumPackets + n]);
    }
}

TEST(delayed_reader, forward_error) {
    const status::StatusCode status_list[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    const stream_timestamp_t delay_list[] = {
        0,
        NumSamples * NumPackets * NsPerSample,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        for (size_t dl_n = 0; dl_n < ROC_ARRAY_SIZE(delay_list); dl_n++) {
            MockReader reader(status_list[st_n]);
            DelayedReader dr(reader, delay_list[dl_n], sample_spec);
            LONGS_EQUAL(status::StatusOK, dr.init_status());

            expect_read(status_list[st_n], dr, ModePeek);
            expect_read(status_list[st_n], dr, ModeFetch);
        }
    }
}

} // namespace packet
} // namespace roc
