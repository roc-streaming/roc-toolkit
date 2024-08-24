/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/fifo_queue.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/identity.h"
#include "roc_rtp/sequencer.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.00001;

enum {
    SamplesPerPacket = 200,
    SampleRate = 1000,

    NumCh = 2,
    ChMask = 0x3,

    MaxPackets = 100,
    MaxBufSize = 4000,

    PayloadType = 123
};

const core::nanoseconds_t PacketDuration = SamplesPerPacket * core::Second / SampleRate;
const core::nanoseconds_t Now = 1691499037871419405;

const SampleSpec frame_spec(
    SampleRate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

const SampleSpec packet_spec(
    SampleRate, PcmSubformat_SInt16_Be, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, MaxBufSize);
FrameFactory frame_factory(arena, MaxBufSize * sizeof(sample_t));

rtp::Composer rtp_composer(NULL, arena);

sample_t nth_sample(uint8_t n) {
    return sample_t(n) / sample_t(1 << 8);
}

FramePtr new_frame(size_t samples_per_chan) {
    FramePtr frame =
        frame_factory.allocate_frame(samples_per_chan * NumCh * sizeof(sample_t));
    CHECK(frame);

    frame->set_raw(true);
    frame->set_duration((packet::stream_timestamp_t)samples_per_chan);

    return frame;
}

class PacketChecker {
public:
    PacketChecker(IFrameDecoder& payload_decoder, core::nanoseconds_t capture_ts = Now)
        : payload_decoder_(payload_decoder)
        , pos_(0)
        , src_(0)
        , sn_(0)
        , ts_(0)
        , capture_ts_(capture_ts)
        , value_(0) {
    }

    void read(packet::IReader& reader, size_t n_samples) {
        packet::PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, reader.read(pp, packet::ModeFetch));
        CHECK(pp);

        UNSIGNED_LONGS_EQUAL(packet::Packet::FlagRTP | packet::Packet::FlagAudio
                                 | packet::Packet::FlagPrepared,
                             pp->flags());

        if (pos_ == 0) {
            src_ = pp->rtp()->source_id;
            sn_ = pp->rtp()->seqnum;
            ts_ = pp->rtp()->stream_timestamp;
        } else {
            UNSIGNED_LONGS_EQUAL(src_, pp->rtp()->source_id);
            UNSIGNED_LONGS_EQUAL(sn_, pp->rtp()->seqnum);
            UNSIGNED_LONGS_EQUAL(ts_, pp->rtp()->stream_timestamp);
        }
        CHECK(core::ns_equal_delta(pp->rtp()->capture_timestamp, capture_ts_,
                                   core::Microsecond));
        if (capture_ts_) {
            capture_ts_ += frame_spec.samples_per_chan_2_ns(n_samples);
        }
        UNSIGNED_LONGS_EQUAL(n_samples, pp->rtp()->duration);
        UNSIGNED_LONGS_EQUAL(PayloadType, pp->rtp()->payload_type);

        CHECK(pp->rtp()->header);
        CHECK(pp->rtp()->payload);

        payload_decoder_.begin_frame(pp->rtp()->stream_timestamp,
                                     pp->rtp()->payload.data(),
                                     pp->rtp()->payload.size());

        sample_t samples[SamplesPerPacket * NumCh] = {};

        LONGS_EQUAL(n_samples, payload_decoder_.read_samples(samples, SamplesPerPacket));

        payload_decoder_.end_frame();

        size_t n = 0;

        for (; n < n_samples; n++) {
            for (size_t c = 0; c < NumCh; c++) {
                DOUBLES_EQUAL((double)nth_sample(value_), (double)samples[n * NumCh + c],
                              Epsilon);
                value_++;
            }
        }

        pos_++;
        sn_++;
        ts_ += n_samples;
    }

private:
    IFrameDecoder& payload_decoder_;

    size_t pos_;

    packet::stream_source_t src_;
    packet::seqnum_t sn_;
    packet::stream_timestamp_t ts_;
    core::nanoseconds_t capture_ts_;

    uint8_t value_;
};

class FrameMaker {
public:
    FrameMaker(core::nanoseconds_t capture_ts = Now)
        : value_(0)
        , capture_ts_(capture_ts) {
    }

    void write(IFrameWriter& writer, size_t samples_per_chan) {
        FramePtr frame = new_frame(samples_per_chan);

        for (size_t n = 0; n < samples_per_chan; n++) {
            for (size_t c = 0; c < NumCh; c++) {
                frame->raw_samples()[n * NumCh + c] = nth_sample(value_);
                value_++;
            }
        }

        frame->set_capture_timestamp(capture_ts_);
        if (capture_ts_) {
            capture_ts_ += frame_spec.samples_per_chan_2_ns(samples_per_chan);
        }

        LONGS_EQUAL(status::StatusOK, writer.write(*frame));
    }

private:
    uint8_t value_;
    core::nanoseconds_t capture_ts_;
};

class StatusWriter : public packet::IWriter {
public:
    explicit StatusWriter(status::StatusCode code)
        : code_(code) {
    }

    virtual status::StatusCode write(const packet::PacketPtr& packet) {
        return code_;
    }

private:
    status::StatusCode code_;
};

} // namespace

TEST_GROUP(packetizer) {};

TEST(packetizer, one_buffer_one_packet) {
    enum { NumFrames = 10 };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue packet_queue;

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_queue, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t fn = 0; fn < NumFrames; fn++) {
        UNSIGNED_LONGS_EQUAL(0, packet_queue.size());

        frame_maker.write(packetizer, SamplesPerPacket);

        UNSIGNED_LONGS_EQUAL(1, packet_queue.size());

        packet_checker.read(packet_queue, SamplesPerPacket);
    }
}

TEST(packetizer, one_buffer_multiple_packets) {
    enum { NumPackets = 10 };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue packet_queue;

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_queue, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    frame_maker.write(packetizer, SamplesPerPacket * NumPackets);

    for (size_t pn = 0; pn < NumPackets; pn++) {
        packet_checker.read(packet_queue, SamplesPerPacket);
    }

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

TEST(packetizer, multiple_buffers_one_packet) {
    enum { NumPackets = 10, FramesPerPacket = 4 };

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue packet_queue;

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_queue, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t pn = 0; pn < NumPackets; pn++) {
        for (size_t fn = 0; fn < FramesPerPacket; fn++) {
            UNSIGNED_LONGS_EQUAL(0, packet_queue.size());

            frame_maker.write(packetizer, SamplesPerPacket / FramesPerPacket);
        }

        UNSIGNED_LONGS_EQUAL(1, packet_queue.size());

        packet_checker.read(packet_queue, SamplesPerPacket);
    }
}

TEST(packetizer, multiple_buffers_multiple_packets) {
    enum {
        NumFrames = 10,
        NumSamples = (SamplesPerPacket - 1),
        NumPackets = (NumSamples * NumFrames / SamplesPerPacket)
    };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue packet_queue;

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_queue, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t fn = 0; fn < NumFrames; fn++) {
        frame_maker.write(packetizer, NumSamples);
    }

    for (size_t pn = 0; pn < NumPackets; pn++) {
        packet_checker.read(packet_queue, SamplesPerPacket);
    }

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

TEST(packetizer, flush) {
    enum { NumIterations = 5, Missing = 10 };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue packet_queue;

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_queue, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t n = 0; n < NumIterations; n++) {
        frame_maker.write(packetizer, SamplesPerPacket);
        frame_maker.write(packetizer, SamplesPerPacket);
        frame_maker.write(packetizer, SamplesPerPacket - Missing);

        UNSIGNED_LONGS_EQUAL(2, packet_queue.size());

        packet_checker.read(packet_queue, SamplesPerPacket);
        packet_checker.read(packet_queue, SamplesPerPacket);

        LONGS_EQUAL(status::StatusOK, packetizer.flush());

        packet_checker.read(packet_queue, SamplesPerPacket - Missing);

        UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
    }
}

TEST(packetizer, timestamp_zero_cts) {
    enum {
        NumFrames = 10,
        NumSamples = (SamplesPerPacket - 1),
        NumPackets = (NumSamples * NumFrames / SamplesPerPacket)
    };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue packet_queue;

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_queue, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    const core::nanoseconds_t zero_cts = 0;

    FrameMaker frame_maker(zero_cts);
    PacketChecker packet_checker(decoder, zero_cts);

    for (size_t fn = 0; fn < NumFrames; fn++) {
        frame_maker.write(packetizer, NumSamples);
    }

    for (size_t pn = 0; pn < NumPackets; pn++) {
        packet_checker.read(packet_queue, SamplesPerPacket);
    }

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

TEST(packetizer, metrics) {
    enum { NumPackets = 10 };

    PcmEncoder encoder(packet_spec, arena);
    packet::FifoQueue packet_queue;

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_queue, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    FrameMaker frame_maker;

    for (size_t pn = 0; pn < NumPackets; pn++) {
        frame_maker.write(packetizer, SamplesPerPacket);

        const PacketizerMetrics metrics = packetizer.metrics();

        UNSIGNED_LONGS_EQUAL(pn + 1, metrics.encoded_packets);
        UNSIGNED_LONGS_EQUAL((pn + 1) * SamplesPerPacket * NumCh * sizeof(int16_t),
                             metrics.payload_bytes);
    }
}

TEST(packetizer, forward_error) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    StatusWriter packet_writer(status::StatusAbort);

    rtp::Identity identity;
    rtp::Sequencer sequencer(identity, PayloadType);
    Packetizer packetizer(packet_writer, rtp_composer, sequencer, encoder, packet_factory,
                          PacketDuration, frame_spec);
    LONGS_EQUAL(status::StatusOK, packetizer.init_status());

    FramePtr frame = new_frame(SamplesPerPacket);
    CHECK(frame);

    LONGS_EQUAL(status::StatusAbort, packetizer.write(*frame));
}

} // namespace audio
} // namespace roc
