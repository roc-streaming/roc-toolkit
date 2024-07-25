/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/status_writer.h"

#include "roc_audio/latency_monitor.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/fast_random.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/config.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/link_meter.h"

namespace roc {
namespace rtp {

namespace {

enum { ChMask = 3, PacketSz = 100, SampleRate = 44100, Duration = 44 };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, PacketSz);

EncodingMap encoding_map(arena);

audio::SampleSpec sample_spec(SampleRate,
                              audio::Sample_RawFormat,
                              audio::ChanLayout_Surround,
                              audio::ChanOrder_Smpte,
                              ChMask);
const core::nanoseconds_t start_ts = 1691499037871419405;
const core::nanoseconds_t step_ts = Duration * core::Second / SampleRate;

const packet::stream_timestamp_t stream_start_ts = 6134803;
const packet::stream_timestamp_t stream_step_ts = Duration;

packet::PacketPtr new_packet(packet::seqnum_t sn,
                             const core::nanoseconds_t queue_ts,
                             const packet::stream_timestamp_t stream_ts) {
    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(packet::Packet::FlagRTP | packet::Packet::FlagUDP);
    packet->rtp()->payload_type = PayloadType_L16_Stereo;
    packet->rtp()->seqnum = sn;
    packet->rtp()->duration = Duration;
    packet->rtp()->stream_timestamp = stream_ts;
    packet->udp()->queue_timestamp = queue_ts;

    return packet;
}

audio::LatencyConfig make_config() {
    audio::LatencyConfig latency_config;
    latency_config.tuner_profile = audio::LatencyTunerProfile_Responsive;
    latency_config.deduce_defaults(pipeline::DefaultLatency, true);
    return latency_config;
}
} // namespace

TEST_GROUP(link_meter) {};

TEST(link_meter, has_metrics) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);

    CHECK(!meter.has_metrics());

    LONGS_EQUAL(status::StatusOK,
                meter.write(new_packet(100, start_ts, stream_start_ts)));
    UNSIGNED_LONGS_EQUAL(1, queue.size());

    CHECK(meter.has_metrics());
}

TEST(link_meter, last_seqnum) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;

    UNSIGNED_LONGS_EQUAL(0, meter.metrics().ext_last_seqnum);

    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(100, ts, sts)));
    UNSIGNED_LONGS_EQUAL(100, meter.metrics().ext_last_seqnum);
    ts += step_ts;
    sts += stream_step_ts;

    // seqnum increased, metric updated
    LONGS_EQUAL(status::StatusOK,
                meter.write(new_packet(102, ts + step_ts, sts + stream_step_ts)));
    UNSIGNED_LONGS_EQUAL(102, meter.metrics().ext_last_seqnum);

    // seqnum decreased, ignored
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(101, ts, sts)));
    UNSIGNED_LONGS_EQUAL(102, meter.metrics().ext_last_seqnum);
    ts += step_ts * 2;
    sts += stream_step_ts * 2;

    // seqnum increased, metric updated
    UNSIGNED_LONGS_EQUAL(status::StatusOK, meter.write(new_packet(103, ts, sts)));

    UNSIGNED_LONGS_EQUAL(0, meter.mean_jitter());

    const packet::LinkMetrics& metrics = meter.metrics();
    UNSIGNED_LONGS_EQUAL(0, metrics.jitter);
    UNSIGNED_LONGS_EQUAL(103, metrics.ext_last_seqnum);

    UNSIGNED_LONGS_EQUAL(4, queue.size());
}

TEST(link_meter, last_seqnum_wrap) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;

    UNSIGNED_LONGS_EQUAL(0, meter.metrics().ext_last_seqnum);

    // no overflow
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(65533, ts, sts)));
    UNSIGNED_LONGS_EQUAL(65533, meter.metrics().ext_last_seqnum);
    UNSIGNED_LONGS_EQUAL(1, meter.metrics().expected_packets);

    // no overflow
    LONGS_EQUAL(
        status::StatusOK,
        meter.write(new_packet(65535, ts + step_ts * 2, sts + stream_step_ts * 2)));
    UNSIGNED_LONGS_EQUAL(65535, meter.metrics().ext_last_seqnum);
    UNSIGNED_LONGS_EQUAL(3, meter.metrics().expected_packets);

    // overflow
    LONGS_EQUAL(status::StatusOK,
                meter.write(new_packet(1, ts + step_ts * 3, sts + stream_step_ts * 3)));
    UNSIGNED_LONGS_EQUAL(65537, meter.metrics().ext_last_seqnum);
    UNSIGNED_LONGS_EQUAL(5, meter.metrics().expected_packets);

    // late packet, ignored
    LONGS_EQUAL(status::StatusOK,
                meter.write(new_packet(65534, ts + step_ts, sts + stream_step_ts)));
    UNSIGNED_LONGS_EQUAL(65537, meter.metrics().ext_last_seqnum);
    UNSIGNED_LONGS_EQUAL(5, meter.metrics().expected_packets);

    // new packet
    LONGS_EQUAL(status::StatusOK,
                meter.write(new_packet(4, ts + step_ts * 6, sts + stream_step_ts * 6)));
    UNSIGNED_LONGS_EQUAL(65540, meter.metrics().ext_last_seqnum);
    UNSIGNED_LONGS_EQUAL(8, meter.metrics().expected_packets);

    UNSIGNED_LONGS_EQUAL(5, queue.size());
}

TEST(link_meter, jitter_test) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);

    const ssize_t running_win_len = (ssize_t)meter.running_window_len();
    const size_t num_packets = Duration * 100;
    core::nanoseconds_t ts_store[num_packets];

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;
    for (size_t i = 0; i < num_packets; i++) {
        packet::seqnum_t seqnum = 65500 + i;
        ts_store[i] = ts;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, meter.write(new_packet(seqnum, ts, sts)));
        const core::nanoseconds_t jitter_ns =
            (core::nanoseconds_t)(core::fast_random_gaussian() * core::Millisecond);
        ts += step_ts + jitter_ns;
        sts += stream_step_ts;

        if (i > (size_t)running_win_len) {
            // Check meter metrics running max in min jitter in last Duration number
            // of packets in ts_store.
            core::nanoseconds_t min_jitter = core::Second;
            core::nanoseconds_t max_jitter = 0;
            for (size_t j = 0; j < (size_t)running_win_len; j++) {
                core::nanoseconds_t jitter =
                    std::abs(ts_store[i - j] - ts_store[i - j - 1] - step_ts);
                min_jitter = std::min(min_jitter, jitter);
                max_jitter = std::max(max_jitter, jitter);
            }
            UNSIGNED_LONGS_EQUAL(min_jitter, meter.metrics().min_jitter);
            UNSIGNED_LONGS_EQUAL(max_jitter, meter.metrics().max_jitter);

            // Reference average  and variance of jitter from ts_store values.
            core::nanoseconds_t sum = 0;
            for (size_t j = 0; j < (size_t)running_win_len; j++) {
                sum += std::abs(ts_store[i - j] - ts_store[i - j - 1] - step_ts);
            }
            const core::nanoseconds_t mean = sum / running_win_len;

            sum = 0;
            for (size_t j = 0; j < (size_t)running_win_len; j++) {
                core::nanoseconds_t jitter =
                    std::abs(ts_store[i - j] - ts_store[i - j - 1] - step_ts);
                sum += (jitter - mean) * (jitter - mean);
            }

            // Check the jitter value
            DOUBLES_EQUAL(mean, meter.mean_jitter(), core::Microsecond * 1);
        }
    }
}

TEST(link_meter, ascending_test) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);
    const ssize_t running_win_len = (ssize_t)meter.running_window_len();

    const size_t num_packets = Duration * 100;
    core::nanoseconds_t ts_store[num_packets];

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;
    for (size_t i = 0; i < num_packets; i++) {
        packet::seqnum_t seqnum = 65500 + i;
        ts_store[i] = ts;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, meter.write(new_packet(seqnum, ts, sts)));
        ts += step_ts + (core::nanoseconds_t)i * core::Microsecond; // Removed the random
                                                                    // component to create
                                                                    // an increasing
                                                                    // sequence
        sts += stream_step_ts;

        if (i > (size_t)running_win_len) {
            // Check meter metrics running max in min jitter in last Duration number
            // of packets in ts_store.
            core::nanoseconds_t min_jitter = core::Second;
            core::nanoseconds_t max_jitter = 0;
            for (size_t j = 0; j < (size_t)running_win_len; j++) {
                core::nanoseconds_t jitter =
                    std::abs(ts_store[i - j] - ts_store[i - j - 1] - step_ts);
                min_jitter = std::min(min_jitter, jitter);
                max_jitter = std::max(max_jitter, jitter);
            }
            UNSIGNED_LONGS_EQUAL(min_jitter, meter.metrics().min_jitter);
            UNSIGNED_LONGS_EQUAL(max_jitter, meter.metrics().max_jitter);
        }
    }
}

TEST(link_meter, descending_test) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);
    const ssize_t RunningWinLen = (ssize_t)meter.running_window_len();

    const size_t num_packets = Duration * 100;
    core::nanoseconds_t ts_store[num_packets];

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;
    for (size_t i = 0; i < num_packets; i++) {
        packet::seqnum_t seqnum = 65500 + i;
        ts_store[i] = ts;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, meter.write(new_packet(seqnum, ts, sts)));
        // Removed the random component to create an decreasing sequence
        ts += step_ts - (core::nanoseconds_t)i * core::Nanosecond * 10;
        sts += stream_step_ts;

        if (i > (size_t)RunningWinLen) {
            // Check meter metrics running max in min jitter in last Duration number
            // of packets in ts_store.
            core::nanoseconds_t min_jitter = core::Second;
            core::nanoseconds_t max_jitter = 0;
            for (size_t j = 0; j < (size_t)RunningWinLen; j++) {
                core::nanoseconds_t jitter =
                    std::abs(ts_store[i - j] - ts_store[i - j - 1] - step_ts);
                min_jitter = std::min(min_jitter, jitter);
                max_jitter = std::max(max_jitter, jitter);
            }
            UNSIGNED_LONGS_EQUAL(min_jitter, meter.metrics().min_jitter);
            UNSIGNED_LONGS_EQUAL(max_jitter, meter.metrics().max_jitter);
        }
    }
}

TEST(link_meter, saw_test) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);
    const ssize_t running_win_len = (ssize_t)meter.running_window_len();

    const size_t num_packets = Duration * 100;
    core::nanoseconds_t ts_store[num_packets];
    core::nanoseconds_t step_ts_inc = core::Nanosecond * 10;
    core::nanoseconds_t step_ts_ = step_ts;

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;
    for (size_t i = 0; i < num_packets; i++) {
        packet::seqnum_t seqnum = 65500 + i;
        ts_store[i] = ts;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, meter.write(new_packet(seqnum, ts, sts)));
        ts += step_ts_;
        sts += stream_step_ts;
        step_ts_ += step_ts_inc;
        if (i > 0 && i % (size_t)running_win_len == 0) {
            step_ts_inc = -step_ts_inc;
        }

        if (i > (size_t)running_win_len) {
            // Check meter metrics running max in min jitter in last Duration number
            // of packets in ts_store.
            core::nanoseconds_t min_jitter = core::Second;
            core::nanoseconds_t max_jitter = 0;
            for (size_t j = 0; j < (size_t)running_win_len; j++) {
                core::nanoseconds_t jitter =
                    std::abs(ts_store[i - j] - ts_store[i - j - 1] - step_ts);
                min_jitter = std::min(min_jitter, jitter);
                max_jitter = std::max(max_jitter, jitter);
            }
            UNSIGNED_LONGS_EQUAL(min_jitter, meter.metrics().min_jitter);
            UNSIGNED_LONGS_EQUAL(max_jitter, meter.metrics().max_jitter);
        }
    }
}

TEST(link_meter, losses_test) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);

    const size_t num_packets = Duration * 2 * (1 << 16);
    int64_t total_losses = 0;

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;
    for (size_t i = 0; i < num_packets; i++) {
        packet::seqnum_t seqnum = 65500 + i;
        packet::PacketPtr p = new_packet(seqnum, ts, sts);
        ts += step_ts;
        sts += stream_step_ts;

        if (i > 0 && core::fast_random_range(0, 100) < 30) {
            i += 99;
            total_losses += 100;
            continue;
        } else {
            UNSIGNED_LONGS_EQUAL(status::StatusOK, meter.write(p));
        }

        packet::PacketPtr pr;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(pr, packet::ModeFetch));
        UNSIGNED_LONGS_EQUAL(pr->rtp()->seqnum, p->rtp()->seqnum);

        if (i > 0) {
            const packet::LinkMetrics& metrics = meter.metrics();
            UNSIGNED_LONGS_EQUAL(total_losses, metrics.lost_packets);
            UNSIGNED_LONGS_EQUAL(i + 1, metrics.expected_packets);
        }
    }
}

TEST(link_meter, total_counter) {
    packet::FifoQueue queue;
    LinkMeter meter(queue, make_config(), encoding_map, arena, NULL);

    core::nanoseconds_t ts = start_ts;
    packet::stream_timestamp_t sts = stream_start_ts;
    uint16_t seqnum = 65500;
    uint32_t total_counter = 0;

    UNSIGNED_LONGS_EQUAL(0, meter.metrics().ext_last_seqnum);

    for (size_t i = 0; i < 66000; i++) {
        LONGS_EQUAL(status::StatusOK,
                    meter.write(new_packet(uint16_t((seqnum + total_counter) & 0xFFFF),
                                           ts + step_ts * total_counter,
                                           sts + stream_step_ts * total_counter)));
        UNSIGNED_LONGS_EQUAL(uint32_t(seqnum) + total_counter,
                             meter.metrics().ext_last_seqnum);
        UNSIGNED_LONGS_EQUAL(total_counter + 1, meter.metrics().expected_packets);

        UNSIGNED_LONGS_EQUAL(i + 1, queue.size());

        total_counter += 1;
    }
}

TEST(link_meter, forward_error) {
    const status::StatusCode status_list[] = {
        status::StatusErrDevice,
        status::StatusErrFile,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        test::StatusWriter writer(status_list[st_n]);
        LinkMeter meter(writer, make_config(), encoding_map, arena, NULL);

        LONGS_EQUAL(status_list[st_n],
                    meter.write(new_packet(100, start_ts, stream_start_ts)));
    }
}

} // namespace rtp
} // namespace roc
