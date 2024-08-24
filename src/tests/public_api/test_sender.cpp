/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/macro_helpers.h"
#include "roc_core/stddefs.h"

#include "roc/config.h"
#include "roc/sender.h"

namespace roc {
namespace api {

TEST_GROUP(sender) {
    roc_sender_config sender_config;

    roc_context* context;

    void setup() {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &context) == 0);
        CHECK(context);

        memset(&sender_config, 0, sizeof(sender_config));
        sender_config.frame_encoding.format = ROC_FORMAT_PCM;
        sender_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
        sender_config.frame_encoding.rate = 44100;
        sender_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
        sender_config.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;
        sender_config.fec_encoding = ROC_FEC_ENCODING_DISABLE;
    }

    void teardown() {
        LONGS_EQUAL(0, roc_context_close(context));
    }
};

TEST(sender, open_close) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
    CHECK(sender);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, connect) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
    CHECK(sender);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint, "127.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint, 123) == 0);

    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, connect_slots) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
    CHECK(sender);

    roc_endpoint* source_endpoint1 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint1) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint1, "rtp://127.0.0.1:111") == 0);

    roc_endpoint* source_endpoint2 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint2) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint2, "rtp://127.0.0.1:222") == 0);

    CHECK(roc_sender_connect(sender, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint1)
          == 0);
    CHECK(roc_sender_connect(sender, 1, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint2)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint1) == 0);
    CHECK(roc_endpoint_deallocate(source_endpoint2) == 0);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, connect_error) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://invalid.:123") == 0);

    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == -1);

    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);
    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == -1);

    CHECK(roc_sender_unlink(sender, ROC_SLOT_DEFAULT) == 0);
    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, configure) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    strcpy(iface_config.outgoing_address, "127.0.0.1");
    iface_config.reuse_address = 1;

    CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                               &iface_config)
          == 0);
    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, configure_defaults) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                               &iface_config)
          == 0);
    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, configure_slots) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
    CHECK(sender);

    roc_endpoint* source_endpoint1 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint1) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint1, "rtp://127.0.0.1:111") == 0);

    roc_endpoint* source_endpoint2 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint2) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint2, "rtp://127.0.0.1:222") == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    CHECK(roc_sender_configure(sender, 0, ROC_INTERFACE_AUDIO_SOURCE, &iface_config)
          == 0);
    CHECK(roc_sender_configure(sender, 1, ROC_INTERFACE_AUDIO_SOURCE, &iface_config)
          == 0);

    CHECK(roc_sender_connect(sender, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint1)
          == 0);
    CHECK(roc_sender_connect(sender, 1, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint2)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint1) == 0);
    CHECK(roc_endpoint_deallocate(source_endpoint2) == 0);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, configure_error) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    strcpy(iface_config.outgoing_address, "8.8.8.8");
    CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                               &iface_config)
          == 0);
    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == -1);

    strcpy(iface_config.outgoing_address, "0.0.0.0");
    CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                               &iface_config)
          == -1);

    CHECK(roc_sender_unlink(sender, ROC_SLOT_DEFAULT) == 0);
    CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                               &iface_config)
          == 0);
    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, unlink) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
    CHECK(sender);

    roc_endpoint* source_endpoint1 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint1) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint1, "rtp://127.0.0.1:111") == 0);

    roc_endpoint* source_endpoint2 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint2) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint2, "rtp://127.0.0.1:222") == 0);

    CHECK(roc_sender_connect(sender, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint1)
          == 0);
    CHECK(roc_sender_connect(sender, 1, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint2)
          == 0);

    CHECK(roc_sender_unlink(sender, 0) == 0);
    CHECK(roc_sender_unlink(sender, 1) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint1) == 0);
    CHECK(roc_endpoint_deallocate(source_endpoint2) == 0);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, unlink_reuse) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
    CHECK(sender);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:111") == 0);

    CHECK(roc_sender_connect(sender, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
          == 0);

    CHECK(roc_sender_unlink(sender, 0) == 0);

    CHECK(roc_sender_connect(sender, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
          == 0);

    CHECK(roc_sender_unlink(sender, 0) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, unlink_error) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
    CHECK(sender);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:111") == 0);

    CHECK(roc_sender_connect(sender, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    CHECK(roc_sender_unlink(sender, 1) == -1);
    CHECK(roc_sender_unlink(sender, 0) == 0);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, bad_args) {
    { // open
        roc_sender* sender = NULL;

        CHECK(roc_sender_open(NULL, &sender_config, &sender) == -1);
        CHECK(roc_sender_open(context, NULL, &sender) == -1);
        CHECK(roc_sender_open(context, &sender_config, NULL) == -1);

        roc_sender_config bad_config;
        memset(&bad_config, 0, sizeof(bad_config));
        CHECK(roc_sender_open(context, &bad_config, &sender) == -1);
    }
    { // close
        CHECK(roc_sender_close(NULL) == -1);
    }
    { // connect
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        CHECK(roc_sender_connect(NULL, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 source_endpoint)
              == -1);
        CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, (roc_interface)-1,
                                 source_endpoint)
              == -1);
        CHECK(
            roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE, NULL)
            == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // configure
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_interface_config iface_config;
        memset(&iface_config, 0, sizeof(iface_config));

        CHECK(roc_sender_configure(NULL, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                   &iface_config)
              == -1);
        CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, (roc_interface)-1,
                                   &iface_config)
              == -1);
        CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                   NULL)
              == -1);

        strcpy(iface_config.outgoing_address, "1.1.1.256");
        CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                   &iface_config)
              == -1);

        strcpy(iface_config.outgoing_address, "2001::eab:dead::a0:abcd:4e");
        CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                   &iface_config)
              == -1);

        strcpy(iface_config.outgoing_address, "bad");
        CHECK(roc_sender_configure(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                   &iface_config)
              == -1);

        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // query
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:111") == 0);

        CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 source_endpoint)
              == 0);

        roc_sender_metrics send_metrics;
        roc_connection_metrics conn_metrics;
        size_t conn_metrics_count = 1;

        // bad
        CHECK(roc_sender_query(NULL, ROC_SLOT_DEFAULT, &send_metrics, &conn_metrics,
                               &conn_metrics_count)
              == -1);
        CHECK(
            roc_sender_query(NULL, 999, &send_metrics, &conn_metrics, &conn_metrics_count)
            == -1);
        CHECK(
            roc_sender_query(sender, ROC_SLOT_DEFAULT, &send_metrics, &conn_metrics, NULL)
            == -1);

        // good
        CHECK(roc_sender_query(sender, ROC_SLOT_DEFAULT, &send_metrics, NULL, NULL) == 0);
        CHECK(roc_sender_query(sender, ROC_SLOT_DEFAULT, NULL, &conn_metrics,
                               &conn_metrics_count)
              == 0);
        CHECK(roc_sender_query(sender, ROC_SLOT_DEFAULT, &send_metrics, NULL,
                               &conn_metrics_count)
              == 0);
        CHECK(roc_sender_query(sender, ROC_SLOT_DEFAULT, &send_metrics, &conn_metrics,
                               &conn_metrics_count)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // unlink
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:111") == 0);

        CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 source_endpoint)
              == 0);

        CHECK(roc_sender_unlink(NULL, ROC_SLOT_DEFAULT) == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
}

TEST(sender, bad_config) {
    { // frame_encoding.rate == 0
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.rate = 0;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // frame_encoding.format == 0
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.format = (roc_format)0;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // frame_encoding.format == 99999
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.format = (roc_format)99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // frame_encoding.channels = 0
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.channels = (roc_channel_layout)0;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // frame_encoding.channels == 99999
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.channels = (roc_channel_layout)99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // frame_encoding.tracks != 0 (non-multitrack)
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.tracks = 1;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // frame_encoding.tracks == 0 (multitrack)
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MULTITRACK;
        sender_config_copy.frame_encoding.tracks = 0;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // frame_encoding.tracks == 99999 (multitrack)
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MULTITRACK;
        sender_config_copy.frame_encoding.tracks = 99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // packet_encoding == 0 (can't select)
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.frame_encoding.rate = 96000;
        sender_config_copy.packet_encoding = (roc_packet_encoding)0;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // packet_encoding == 99999
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.packet_encoding = (roc_packet_encoding)99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // fec_encoding == 99999
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.fec_encoding = (roc_fec_encoding)99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // clock_source == 99999
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.clock_source = (roc_clock_source)99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // resampler_backend == 99999
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.resampler_backend = (roc_resampler_backend)99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
    { // resampler_profile == 99999
        roc_sender_config sender_config_copy = sender_config;
        sender_config_copy.resampler_profile = (roc_resampler_profile)99999;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config_copy, &sender) != 0);
        CHECK(!sender);
    }
}

TEST(sender, write_args) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    float samples[16] = {};

    { // all good, not connected
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_sender_write(sender, &frame) == 0);
    }

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

    CHECK(roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                             source_endpoint)
          == 0);

    { // all good, connected
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_sender_write(sender, &frame) == 0);
    }

    { // null sender
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_sender_write(NULL, &frame) == -1);
    }

    { // null frame
        CHECK(roc_sender_write(sender, NULL) == -1);
    }

    { // null samples, zero sample count
        roc_frame frame;
        frame.samples = NULL;
        frame.samples_size = 0;
        CHECK(roc_sender_write(sender, &frame) == 0);
    }

    { // null samples, non-zero sample count
        roc_frame frame;
        frame.samples = NULL;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_sender_write(sender, &frame) == -1);
    }

    { // uneven sample count
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = 1;
        CHECK(roc_sender_write(sender, &frame) == -1);
    }

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

} // namespace api
} // namespace roc
