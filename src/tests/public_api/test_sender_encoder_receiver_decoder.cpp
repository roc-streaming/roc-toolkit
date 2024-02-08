/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
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

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, test::MaxBufSize);

} // namespace

TEST_GROUP(sender_encoder_receiver_decoder) {
    roc_sender_config sender_conf;
    roc_receiver_config receiver_conf;

    roc_context* context;

    void setup() {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &context) == 0);
        CHECK(context);

        memset(&sender_conf, 0, sizeof(sender_conf));
        sender_conf.frame_encoding.rate = test::SampleRate;
        sender_conf.frame_encoding.format = ROC_FORMAT_PCM_FLOAT32;
        sender_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
        sender_conf.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;
        sender_conf.packet_length =
            test::PacketSamples * 1000000000ull / test::SampleRate;
        sender_conf.clock_source = ROC_CLOCK_SOURCE_EXTERNAL;

        memset(&receiver_conf, 0, sizeof(receiver_conf));
        receiver_conf.frame_encoding.rate = test::SampleRate;
        receiver_conf.frame_encoding.format = ROC_FORMAT_PCM_FLOAT32;
        receiver_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
        receiver_conf.clock_source = ROC_CLOCK_SOURCE_EXTERNAL;
        receiver_conf.latency_tuner_profile = ROC_LATENCY_TUNER_PROFILE_INTACT;
        receiver_conf.target_latency = test::Latency * 1000000000ull / test::SampleRate;
        receiver_conf.no_playback_timeout =
            test::Timeout * 1000000000ull / test::SampleRate;
    }

    void teardown() {
        LONGS_EQUAL(0, roc_context_close(context));
    }

    bool is_rs8m_supported() {
        return fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8);
    }

    bool is_zero(float s) {
        return std::abs(s) < 1e-6f;
    }

    void run_test(roc_sender_encoder * encoder, roc_receiver_decoder * decoder,
                  const roc_interface* ifaces, size_t num_ifaces) {
        enum {
            NumFrames = test::Latency / test::FrameSamples * 50,
            MaxLeadingZeros = test::Latency * 2
        };

        const float sample_step = 1. / 32768.;

        float send_value = sample_step, recv_value = 0;
        bool leading_zeros = true;

        size_t iface_packets[10] = {};
        size_t zero_samples = 0, total_samples = 0;

        unsigned long long max_e2e_latency = 0;

        bool has_control = false;

        for (size_t n_if = 0; n_if < num_ifaces; n_if++) {
            if (ifaces[n_if] == ROC_INTERFACE_AUDIO_CONTROL) {
                has_control = true;
            }
        }

        for (size_t nf = 0; nf < NumFrames; nf++) {
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
            {
                // simulate small network delay, so that receiver will calculate
                // non-zero latency
                core::sleep_for(core::ClockMonotonic, core::Microsecond * 50);
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

                        CHECK(roc_receiver_decoder_push_packet(decoder, ifaces[n_if],
                                                               &packet)
                              == 0);

                        iface_packets[n_if]++;
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
            { // check metrics
                roc_receiver_metrics recv_metrics;
                memset(&recv_metrics, 0, sizeof(recv_metrics));
                roc_connection_metrics conn_metrics;
                memset(&conn_metrics, 0, sizeof(conn_metrics));
                size_t conn_metrics_count = 1;

                CHECK(roc_receiver_decoder_query(decoder, &recv_metrics, &conn_metrics,
                                                 &conn_metrics_count)
                      == 0);

                UNSIGNED_LONGS_EQUAL(1, recv_metrics.connection_count);
                UNSIGNED_LONGS_EQUAL(1, conn_metrics_count);

                max_e2e_latency = std::max(max_e2e_latency, conn_metrics.e2e_latency);
            }
        }

        // check we have received enough good samples
        CHECK(zero_samples < MaxLeadingZeros);

        // check that there were packets on all active interfaces
        for (size_t n_if = 0; n_if < num_ifaces; n_if++) {
            CHECK(iface_packets[n_if] > 0);
        }

        // check metrics
        if (has_control) {
            CHECK(max_e2e_latency > 0);
        } else {
            CHECK(max_e2e_latency == 0);
        }
    }
};

TEST(sender_encoder_receiver_decoder, source) {
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

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces));

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(sender_encoder_receiver_decoder, source_control) {
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

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces));

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(sender_encoder_receiver_decoder, source_repair) {
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

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces));

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(sender_encoder_receiver_decoder, source_repair_control) {
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

    run_test(encoder, decoder, ifaces, ROC_ARRAY_SIZE(ifaces));

    LONGS_EQUAL(0, roc_sender_encoder_close(encoder));
    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

} // namespace api
} // namespace roc
