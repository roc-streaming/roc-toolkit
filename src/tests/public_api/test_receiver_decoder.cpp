/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/macro_helpers.h"
#include "roc_core/stddefs.h"

#include "roc/config.h"
#include "roc/receiver_decoder.h"

namespace roc {
namespace api {

TEST_GROUP(receiver_decoder) {
    roc_receiver_config receiver_config;

    roc_context* context;

    void setup() {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &context) == 0);
        CHECK(context);

        memset(&receiver_config, 0, sizeof(receiver_config));
        receiver_config.frame_encoding.rate = 44100;
        receiver_config.frame_encoding.format = ROC_FORMAT_PCM_FLOAT32;
        receiver_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
    }

    void teardown() {
        LONGS_EQUAL(0, roc_context_close(context));
    }
};

TEST(receiver_decoder, open_close) {
    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_config, &decoder) == 0);
    CHECK(decoder);

    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(receiver_decoder, activate) {
    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_config, &decoder) == 0);
    CHECK(decoder);

    CHECK(
        roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
        == 0);

    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(receiver_decoder, activate_error) {
    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_config, &decoder) == 0);
    CHECK(decoder);

    CHECK(
        roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
        == 0);

    CHECK(
        roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
        == -1);

    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(receiver_decoder, bad_args) {
    { // open
        roc_receiver_decoder* decoder = NULL;

        CHECK(roc_receiver_decoder_open(NULL, &receiver_config, &decoder) == -1);
        CHECK(roc_receiver_decoder_open(context, NULL, &decoder) == -1);
        CHECK(roc_receiver_decoder_open(context, &receiver_config, NULL) == -1);

        roc_receiver_config bad_config;
        memset(&bad_config, 0, sizeof(bad_config));
        CHECK(roc_receiver_decoder_open(context, &bad_config, &decoder) == -1);
    }
    { // close
        CHECK(roc_receiver_decoder_close(NULL) == -1);
    }
    { // activate
        roc_receiver_decoder* decoder = NULL;
        CHECK(roc_receiver_decoder_open(context, &receiver_config, &decoder) == 0);

        CHECK(
            roc_receiver_decoder_activate(NULL, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
            == -1);
        CHECK(roc_receiver_decoder_activate(decoder, (roc_interface)-1, ROC_PROTO_RTP)
              == -1);
        CHECK(roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE,
                                            (roc_protocol)-1)
              == -1);

        LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
    }
    { // query
        roc_receiver_decoder* decoder = NULL;
        CHECK(roc_receiver_decoder_open(context, &receiver_config, &decoder) == 0);

        roc_receiver_metrics recv_metrics;
        roc_connection_metrics conn_metrics;
        size_t conn_metrics_count = 1;

        // bad
        CHECK(roc_receiver_decoder_query(NULL, &recv_metrics, &conn_metrics,
                                         &conn_metrics_count)
              == -1);
        CHECK(roc_receiver_decoder_query(decoder, &recv_metrics, &conn_metrics, NULL)
              == -1);

        // good
        CHECK(roc_receiver_decoder_query(decoder, &recv_metrics, NULL, NULL) == 0);
        CHECK(
            roc_receiver_decoder_query(decoder, NULL, &conn_metrics, &conn_metrics_count)
            == 0);
        CHECK(
            roc_receiver_decoder_query(decoder, &recv_metrics, NULL, &conn_metrics_count)
            == 0);
        CHECK(roc_receiver_decoder_query(decoder, &recv_metrics, &conn_metrics,
                                         &conn_metrics_count)
              == 0);

        LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
    }
}

TEST(receiver_decoder, push_args) {
    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_config, &decoder) == 0);

    CHECK(
        roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
        == 0);

    float bytes[256] = {};

    { // null decoder
        roc_packet packet;
        packet.bytes = bytes;
        packet.bytes_size = ROC_ARRAY_SIZE(bytes);
        CHECK(roc_receiver_decoder_push_packet(NULL, ROC_INTERFACE_AUDIO_SOURCE, &packet)
              == -1);
    }

    { // bad interface
        roc_packet packet;
        packet.bytes = bytes;
        packet.bytes_size = ROC_ARRAY_SIZE(bytes);
        CHECK(roc_receiver_decoder_push_packet(decoder, (roc_interface)-1, &packet)
              == -1);
    }

    { // inactive interface
        roc_packet packet;
        packet.bytes = bytes;
        packet.bytes_size = ROC_ARRAY_SIZE(bytes);
        CHECK(
            roc_receiver_decoder_push_packet(decoder, ROC_INTERFACE_AUDIO_REPAIR, &packet)
            == -1);
    }

    { // null packet
        CHECK(roc_receiver_decoder_push_packet(decoder, ROC_INTERFACE_AUDIO_SOURCE, NULL)
              == -1);
    }

    { // null bytes, non-zero byte count
        roc_packet packet;
        packet.bytes = NULL;
        packet.bytes_size = ROC_ARRAY_SIZE(bytes);
        CHECK(
            roc_receiver_decoder_push_packet(decoder, ROC_INTERFACE_AUDIO_SOURCE, &packet)
            == -1);
    }

    { // zero byte count
        roc_packet packet;
        packet.bytes = bytes;
        packet.bytes_size = 0;
        CHECK(
            roc_receiver_decoder_push_packet(decoder, ROC_INTERFACE_AUDIO_SOURCE, &packet)
            == -1);
    }

    { // large byte count
        float large_bytes[10000] = {};
        roc_packet packet;
        packet.bytes = large_bytes;
        packet.bytes_size = ROC_ARRAY_SIZE(large_bytes);
        CHECK(
            roc_receiver_decoder_push_packet(decoder, ROC_INTERFACE_AUDIO_SOURCE, &packet)
            == -1);
    }

    { // all good
        roc_packet packet;
        packet.bytes = bytes;
        packet.bytes_size = ROC_ARRAY_SIZE(bytes);
        CHECK(
            roc_receiver_decoder_push_packet(decoder, ROC_INTERFACE_AUDIO_SOURCE, &packet)
            == 0);
    }

    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

TEST(receiver_decoder, pop_args) {
    roc_receiver_decoder* decoder = NULL;
    CHECK(roc_receiver_decoder_open(context, &receiver_config, &decoder) == 0);

    float samples[16] = {};

    { // all good, not bound
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_decoder_pop_frame(decoder, &frame) == 0);
    }

    CHECK(
        roc_receiver_decoder_activate(decoder, ROC_INTERFACE_AUDIO_SOURCE, ROC_PROTO_RTP)
        == 0);

    { // all good, bound
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_decoder_pop_frame(decoder, &frame) == 0);
    }

    { // null decoder
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_decoder_pop_frame(NULL, &frame) == -1);
    }

    { // null frame
        CHECK(roc_receiver_decoder_pop_frame(decoder, NULL) == -1);
    }

    { // null samples, zero sample count
        roc_frame frame;
        frame.samples = NULL;
        frame.samples_size = 0;
        CHECK(roc_receiver_decoder_pop_frame(decoder, &frame) == 0);
    }

    { // null samples, non-zero sample count
        roc_frame frame;
        frame.samples = NULL;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_decoder_pop_frame(decoder, &frame) == -1);
    }

    { // uneven sample count
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = 1;
        CHECK(roc_receiver_decoder_pop_frame(decoder, &frame) == -1);
    }

    LONGS_EQUAL(0, roc_receiver_decoder_close(decoder));
}

} // namespace api
} // namespace roc
