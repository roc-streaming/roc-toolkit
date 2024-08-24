/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/macro_helpers.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/packet_factory.h"

#include "roc/config.h"
#include "roc/receiver_decoder.h"
#include "roc/sender_encoder.h"

namespace roc {
namespace api {

namespace {

enum {
    SampleRate = 44100,
    NoFlags = 0,
    FlagLosses = (1 << 0),
};

} // namespace

TEST_GROUP(loopback_encoder_2_decoder) {
    roc_sender_config sender_conf;
    roc_receiver_config receiver_conf;

    roc_context* context;

    void setup() {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &context) == 0);
        CHECK(context);

        memset(&sender_conf, 0, sizeof(sender_conf));
        sender_conf.frame_encoding.format = ROC_FORMAT_PCM;
        sender_conf.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
        sender_conf.frame_encoding.rate = SampleRate;
        sender_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
        sender_conf.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;
        sender_conf.packet_length = test::PacketSamples * 1000000000ull / SampleRate;
        sender_conf.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

        memset(&receiver_conf, 0, sizeof(receiver_conf));
        receiver_conf.frame_encoding.format = ROC_FORMAT_PCM;
        receiver_conf.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
        receiver_conf.frame_encoding.rate = SampleRate;
        receiver_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
        receiver_conf.clock_source = ROC_CLOCK_SOURCE_INTERNAL;
        receiver_conf.latency_tuner_profile = ROC_LATENCY_TUNER_PROFILE_INTACT;
        receiver_conf.target_latency = test::Latency * 1000000000ull / SampleRate;
        receiver_conf.no_playback_timeout = test::Timeout * 1000000000ull / SampleRate;
    }

    void teardown() {
        LONGS_EQUAL(0, roc_context_close(context));
    }

    bool is_rs8m_supported() {
        return fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8);
    }

    bool is_zero(float s) {
        return std::abs(s) < 1e-6f;
    }

    void run_test(roc_sender_encoder * encoder, roc_receiver_decoder * decoder,
                  const roc_interface* ifaces, size_t num_ifaces, int flags) {
        enum {
            NumFrames = test::Latency * 10 / test::FrameSamples,
            MaxLeadingZeros = test::Latency * 2,
            LossRatio = 5,
        };

        const float sample_step = 1. / 32768.;

        float send_value = sample_step, recv_value = 0;
        bool leading_zeros = true;

        size_t iface_packets[10] = {};
        size_t recv_expected_pkts = 0;
        uint64_t recv_lost_pkts = 0;
        uint64_t recv_late_pkts = 0;
        uint64_t recv_recovered_pkts = 0;
        size_t send_expected_pkts = 0;
        size_t send_lost_pkts = 0;
        size_t feedback_packets = 0;
        size_t zero_samples = 0, total_samples = 0;
        size_t n_pkt = 0;
        size_t n_lost = 0;

        unsigned long long max_recv_e2e_latency = 0;
        unsigned long long max_send_e2e_latency = 0;

        bool has_control = false;
        bool got_all_metrics = false;

        for (size_t n_if = 0; n_if < num_ifaces; n_if++) {
            if (ifaces[n_if] == ROC_INTERFACE_AUDIO_CONTROL) {
                has_control = true;
            }
        }

        const size_t last_frame = NumFrames - 1;
        for (size_t nf = 0; nf <= last_frame || !got_all_metrics; nf++) {
            { // write frame to encoder
                float samples[test::FrameSamples] = {};

                for (size_t ns = 0; ns < test::FrameSamples; ns++) {
                    samples[ns] = send_value;
                    send_value = test::increment_sample_value(send_value, sample_step);
                }

                roc_frame frame;
                frame.samples = samples;
                frame.samples_size = test::FrameSamples * sizeof(float);
                CHECK(roc_sender_encoder_push_frame(encoder, &frame) == 0);
            }
            { // read encoded packets from encoder and write to decoder
                uint8_t bytes[test::MaxBufSize] = {};

                // repeat for all enabled interfaces (source, repair, etc)
                for (size_t n_if = 0; n_if < num_ifaces; n_if++) {
                    for (;;) {
                        roc_packet packet;
                        packet.bytes = bytes;
                        packet.bytes_size = test::MaxBufSize;

                        if (roc_sender_encoder_pop_packet(encoder, ifaces[n_if], &packet)
                            != 0) {
                            break;
                        }

                        const bool loss = (flags & FlagLosses)
                            && (ifaces[n_if] == ROC_INTERFACE_AUDIO_SOURCE)
                            && ((n_pkt + 3) % LossRatio == 0) && nf < last_frame;

                        if (!loss) {
                            CHECK(roc_receiver_decoder_push_packet(decoder, ifaces[n_if],
                                                                   &packet)
                                  == 0);
                        } else {
                            n_lost++;
                        }

                        iface_packets[n_if]++;
                        n_pkt++;
                    }
                }
            }
            { // read encoded feedback packets from decoder and write to encoder
                uint8_t bytes[test::MaxBufSize] = {};

                if (has_control) {
                    for (;;) {
                        roc_packet packet;
                        packet.bytes = bytes;
                        packet.bytes_size = test::MaxBufSize;

                        if (roc_receiver_decoder_pop_feedback_packet(
                                decoder, ROC_INTERFACE_AUDIO_CONTROL, &packet)
                            != 0) {
                            break;
                        }

                        CHECK(roc_sender_encoder_push_feedback_packet(
                                  encoder, ROC_INTERFACE_AUDIO_CONTROL, &packet)
                              == 0);

                        feedback_packets++;
                    }
                }
            }
            { // read frame from decoder
                float samples[test::FrameSamples] = {};

                roc_frame frame;
                frame.samples = samples;
                frame.samples_size = test::FrameSamples * sizeof(float);
                CHECK(roc_receiver_decoder_pop_frame(decoder, &frame) == 0);

                for (size_t ns = 0; ns < test::FrameSamples; ns++) {
                    total_samples++;

                    if (leading_zeros && !is_zero(samples[ns])) {
                        leading_zeros = false;
                        recv_value = samples[ns];
                    }

                    if (leading_zeros) {
                        zero_samples++;
                    } else {
                        if (!is_zero(recv_value - samples[ns])) {
                            char sbuff[256];
                            snprintf(sbuff, sizeof(sbuff),
                                     "failed comparing samples:\n\n"
                                     "frame_num: %lu, frame_off: %lu\n"
                                     "zero_samples: %lu, total_samples: %lu\n"
                                     "expected: %f, received: %f\n",
                                     (unsigned long)nf, (unsigned long)ns,
                                     (unsigned long)zero_samples,
                                     (unsigned long)total_samples, (double)recv_value,
                                     (double)samples[ns]);
                            FAIL(sbuff);
                        }
                        recv_value =
                            test::increment_sample_value(recv_value, sample_step);
                    }
                }
            }
            { // check receiver metrics
                roc_receiver_metrics recv_metrics;
                memset(&recv_metrics, 0, sizeof(recv_metrics));
                roc_connection_metrics conn_metrics;
                memset(&conn_metrics, 0, sizeof(conn_metrics));

                CHECK(roc_receiver_decoder_query(decoder, &recv_metrics, &conn_metrics)
                      == 0);

                UNSIGNED_LONGS_EQUAL(1, recv_metrics.connection_count);

                max_recv_e2e_latency =
                    std::max(max_recv_e2e_latency, conn_metrics.e2e_latency);

                CHECK(conn_metrics.expected_packets >= recv_expected_pkts);
                recv_expected_pkts = conn_metrics.expected_packets;

                CHECK(conn_metrics.lost_packets >= recv_lost_pkts);
                CHECK(conn_metrics.lost_packets <= conn_metrics.expected_packets);
                recv_lost_pkts = conn_metrics.lost_packets;

                CHECK(conn_metrics.late_packets >= recv_late_pkts);
                CHECK(conn_metrics.late_packets <= conn_metrics.expected_packets);
                recv_late_pkts = conn_metrics.late_packets;

                CHECK(conn_metrics.recovered_packets >= recv_recovered_pkts);
                CHECK(conn_metrics.recovered_packets <= conn_metrics.expected_packets);
                recv_recovered_pkts = conn_metrics.recovered_packets;
            }
            { // check sender metrics
                roc_sender_metrics send_metrics;
                memset(&send_metrics, 0, sizeof(send_metrics));
                roc_connection_metrics conn_metrics;
                memset(&conn_metrics, 0, sizeof(conn_metrics));

                CHECK(roc_sender_encoder_query(encoder, &send_metrics, &conn_metrics)
                      == 0);

                if (send_metrics.connection_count != 0) {
                    UNSIGNED_LONGS_EQUAL(1, send_metrics.connection_count);

                    max_send_e2e_latency =
                        std::max(max_send_e2e_latency, conn_metrics.e2e_latency);

                    CHECK(conn_metrics.expected_packets >= send_expected_pkts);
                    send_expected_pkts = conn_metrics.expected_packets;

                    CHECK(conn_metrics.lost_packets >= send_lost_pkts);
                    CHECK(conn_metrics.lost_packets <= conn_metrics.expected_packets);
                    send_lost_pkts = conn_metrics.lost_packets;
                }
            }

            if (has_control) {
                got_all_metrics = max_recv_e2e_latency > 0 && max_send_e2e_latency > 0
                    && recv_lost_pkts >= n_lost && send_lost_pkts >= n_lost;
            } else {
                got_all_metrics = true;
            }
        }

        // ensure that we have received enough good samples
        CHECK(zero_samples < MaxLeadingZeros);

        // check that there were packets on all active interfaces
        for (size_t n_if = 0; n_if < num_ifaces; n_if++) {
            CHECK(iface_packets[n_if] > 0);
        }

        // check feedback packets
        if (has_control) {
            CHECK(feedback_packets > 0);
        } else {
            CHECK(feedback_packets == 0);
        }

        // check packet counters: expected_packets
        for (size_t n_if = 0; n_if < num_ifaces; n_if++) {
            if (ifaces[n_if] == ROC_INTERFACE_AUDIO_SOURCE) {
                UNSIGNED_LONGS_EQUAL(iface_packets[n_if], recv_expected_pkts);
                if (has_control) {
                    const size_t nlag = test::FrameSamples / test::PacketSamples;
                    CHECK(recv_expected_pkts >= send_expected_pkts
                          && recv_expected_pkts <= send_expected_pkts + nlag);
                }
            }
        }

        // check packet counters: late_packets, lost_packets, recovered_packets
        UNSIGNED_LONGS_EQUAL(0, recv_late_pkts);
        UNSIGNED_LONGS_EQUAL(n_lost, recv_lost_pkts);
        if (has_control) {
            UNSIGNED_LONGS_EQUAL(n_lost, send_lost_pkts);
        } else {
            UNSIGNED_LONGS_EQUAL(0, send_lost_pkts);
        }
        if (flags & FlagLosses) {
            CHECK(n_lost > 0);
            CHECK(n_lost < recv_expected_pkts);
            CHECK(recv_recovered_pkts > 0);
            CHECK(recv_recovered_pkts <= n_lost);
        } else {
            CHECK(n_lost == 0);
            CHECK(recv_recovered_pkts == 0);
        }

        // check measured latency
        if (has_control) {
            CHECK(max_recv_e2e_latency > 0);
            CHECK(max_send_e2e_latency > 0);
        } else {
            CHECK(max_recv_e2e_latency == 0);
            CHECK(max_send_e2e_latency == 0);
        }
    }
};

TEST(loopback_encoder_2_decoder, source) {
    sender_conf.fec_encoding = ROC_FEC_ENCODING_DISABLE;

    roc_sender_encoder* encoder = NULL;
    CHECK(roc_sender_encoder_open(context, &sender_conf, &encoder) == 0);
    CHECK(encoder);

    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_conf, &decoder) == 0);
    CHECK(decoder);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
          == 0);

    CHECK(
        roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
        == 0);

    roc_interface ifaces[] = {
        ROC_INTERFACE_AUDIO_SOURCE,
    };

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces), NoFlags);

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(loopback_encoder_2_decoder, source_control) {
    enum { Flags = 0 };

    sender_conf.fec_encoding = ROC_FEC_ENCODING_DISABLE;

    roc_sender_encoder* encoder = NULL;
    CHECK(roc_sender_encoder_open(context, &sender_conf, &encoder) == 0);
    CHECK(encoder);

    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_conf, &decoder) == 0);
    CHECK(decoder);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
          == 0);

    CHECK(
        roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_CONTROL, ROC_PROTO_RTCP)
        == 0);

    CHECK(
        roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
        == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_CONTROL,
                                        ROC_PROTO_RTCP)
          == 0);

    roc_interface ifaces[] = {
        ROC_INTERFACE_AUDIO_SOURCE,
        ROC_INTERFACE_AUDIO_CONTROL,
    };

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces), NoFlags);

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(loopback_encoder_2_decoder, source_repair) {
    if (!is_rs8m_supported()) {
        return;
    }

    sender_conf.fec_encoding = ROC_FEC_ENCODING_RS8M;
    sender_conf.fec_block_source_packets = test::SourcePackets;
    sender_conf.fec_block_repair_packets = test::RepairPackets;

    roc_sender_encoder* encoder = NULL;
    CHECK(roc_sender_encoder_open(context, &sender_conf, &encoder) == 0);
    CHECK(encoder);

    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_conf, &decoder) == 0);
    CHECK(decoder);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_SOURCE,
                                      ROC_PROTO_RTP_RS8M_SOURCE)
          == 0);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_REPAIR,
                                      ROC_PROTO_RS8M_REPAIR)
          == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE,
                                        ROC_PROTO_RTP_RS8M_SOURCE)
          == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_REPAIR,
                                        ROC_PROTO_RS8M_REPAIR)
          == 0);

    roc_interface ifaces[] = {
        ROC_INTERFACE_AUDIO_SOURCE,
        ROC_INTERFACE_AUDIO_REPAIR,
    };

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces), NoFlags);

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(loopback_encoder_2_decoder, source_repair_losses) {
    if (!is_rs8m_supported()) {
        return;
    }

    sender_conf.fec_encoding = ROC_FEC_ENCODING_RS8M;
    sender_conf.fec_block_source_packets = test::SourcePackets;
    sender_conf.fec_block_repair_packets = test::RepairPackets;

    roc_sender_encoder* encoder = NULL;
    CHECK(roc_sender_encoder_open(context, &sender_conf, &encoder) == 0);
    CHECK(encoder);

    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_conf, &decoder) == 0);
    CHECK(decoder);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_SOURCE,
                                      ROC_PROTO_RTP_RS8M_SOURCE)
          == 0);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_REPAIR,
                                      ROC_PROTO_RS8M_REPAIR)
          == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE,
                                        ROC_PROTO_RTP_RS8M_SOURCE)
          == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_REPAIR,
                                        ROC_PROTO_RS8M_REPAIR)
          == 0);

    roc_interface ifaces[] = {
        ROC_INTERFACE_AUDIO_SOURCE,
        ROC_INTERFACE_AUDIO_REPAIR,
    };

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces), FlagLosses);

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(loopback_encoder_2_decoder, source_repair_control) {
    if (!is_rs8m_supported()) {
        return;
    }

    sender_conf.fec_encoding = ROC_FEC_ENCODING_RS8M;
    sender_conf.fec_block_source_packets = test::SourcePackets;
    sender_conf.fec_block_repair_packets = test::RepairPackets;

    roc_sender_encoder* encoder = NULL;
    CHECK(roc_sender_encoder_open(context, &sender_conf, &encoder) == 0);
    CHECK(encoder);

    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_conf, &decoder) == 0);
    CHECK(decoder);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_SOURCE,
                                      ROC_PROTO_RTP_RS8M_SOURCE)
          == 0);

    CHECK(roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_REPAIR,
                                      ROC_PROTO_RS8M_REPAIR)
          == 0);

    CHECK(
        roc_sender_encoder_activate(encoder, ROC_INTERFACE_AUDIO_CONTROL, ROC_PROTO_RTCP)
        == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE,
                                        ROC_PROTO_RTP_RS8M_SOURCE)
          == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_REPAIR,
                                        ROC_PROTO_RS8M_REPAIR)
          == 0);

    CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_CONTROL,
                                        ROC_PROTO_RTCP)
          == 0);

    roc_interface ifaces[] = {
        ROC_INTERFACE_AUDIO_SOURCE,
        ROC_INTERFACE_AUDIO_REPAIR,
        ROC_INTERFACE_AUDIO_CONTROL,
    };

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces), NoFlags);

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

} // namespace api
} // namespace roc
