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
#include "roc/receiver.h"

namespace roc {
namespace api {

TEST_GROUP(receiver) {
    roc_receiver_config receiver_config;

    roc_context* context;

    void setup() {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &context) == 0);
        CHECK(context);

        memset(&receiver_config, 0, sizeof(receiver_config));
        receiver_config.frame_encoding.format = ROC_FORMAT_PCM;
        receiver_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
        receiver_config.frame_encoding.rate = 44100;
        receiver_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
    }

    void teardown() {
        LONGS_EQUAL(0, roc_context_close(context));
    }
};

TEST(receiver, open_close) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, bind) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint, "127.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint, 0) == 0);

    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, bind_slots) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint1 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint1) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint1, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint1, "127.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint1, 0) == 0);

    roc_endpoint* source_endpoint2 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint2) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint2, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint2, "127.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint2, 0) == 0);

    CHECK(roc_receiver_bind(receiver, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint1)
          == 0);
    CHECK(roc_receiver_bind(receiver, 1, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint2)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint1) == 0);
    CHECK(roc_endpoint_deallocate(source_endpoint2) == 0);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, bind_error) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://invalid.:0") == 0);

    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == -1);

    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);
    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == -1);

    CHECK(roc_receiver_unlink(receiver, ROC_SLOT_DEFAULT) == 0);
    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, configure) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint, "224.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint, 0) == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    strcpy(iface_config.multicast_group, "0.0.0.0");
    iface_config.reuse_address = 1;

    CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 &iface_config)
          == 0);
    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, configure_defaults) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint, "224.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint, 0) == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 &iface_config)
          == 0);
    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, configure_slots) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint1 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint1) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint1, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint1, "224.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint1, 0) == 0);

    roc_endpoint* source_endpoint2 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint2) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint2, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint2, "224.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint2, 0) == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    CHECK(roc_receiver_configure(receiver, 0, ROC_INTERFACE_AUDIO_SOURCE, &iface_config)
          == 0);
    CHECK(roc_receiver_configure(receiver, 1, ROC_INTERFACE_AUDIO_SOURCE, &iface_config)
          == 0);

    CHECK(roc_receiver_bind(receiver, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint1)
          == 0);
    CHECK(roc_receiver_bind(receiver, 1, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint2)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint1) == 0);
    CHECK(roc_endpoint_deallocate(source_endpoint2) == 0);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, configure_error) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://224.0.0.1:0") == 0);

    roc_interface_config iface_config;
    memset(&iface_config, 0, sizeof(iface_config));

    strcpy(iface_config.multicast_group, "8.8.8.8");

    CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 &iface_config)
          == 0);
    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == -1);

    strcpy(iface_config.multicast_group, "0.0.0.0");
    CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 &iface_config)
          == -1);

    CHECK(roc_receiver_unlink(receiver, ROC_SLOT_DEFAULT) == 0);
    CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                 &iface_config)
          == 0);
    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, unlink) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint1 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint1) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint1, "rtp://127.0.0.1:0") == 0);

    roc_endpoint* source_endpoint2 = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint2) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint2, "rtp://127.0.0.1:0") == 0);

    CHECK(roc_receiver_bind(receiver, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint1)
          == 0);
    CHECK(roc_receiver_bind(receiver, 1, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint2)
          == 0);

    CHECK(roc_receiver_unlink(receiver, 0) == 0);
    CHECK(roc_receiver_unlink(receiver, 1) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint1) == 0);
    CHECK(roc_endpoint_deallocate(source_endpoint2) == 0);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, unlink_reuse) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

    CHECK(roc_receiver_bind(receiver, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
          == 0);

    CHECK(roc_receiver_unlink(receiver, 0) == 0);

    CHECK(roc_receiver_bind(receiver, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
          == 0);

    CHECK(roc_receiver_unlink(receiver, 0) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, unlink_error) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

    CHECK(roc_receiver_bind(receiver, 0, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
          == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    CHECK(roc_receiver_unlink(receiver, 1) == -1);
    CHECK(roc_receiver_unlink(receiver, 0) == 0);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, bad_args) {
    { // open
        roc_receiver* receiver = NULL;

        CHECK(roc_receiver_open(NULL, &receiver_config, &receiver) == -1);
        CHECK(roc_receiver_open(context, NULL, &receiver) == -1);
        CHECK(roc_receiver_open(context, &receiver_config, NULL) == -1);

        roc_receiver_config bad_config;
        memset(&bad_config, 0, sizeof(bad_config));
        CHECK(roc_receiver_open(context, &bad_config, &receiver) == -1);
    }
    { // close
        CHECK(roc_receiver_close(NULL) == -1);
    }
    { // bind
        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        CHECK(roc_receiver_bind(NULL, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                source_endpoint)
              == -1);
        CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, (roc_interface)-1,
                                source_endpoint)
              == -1);
        CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                NULL)
              == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // configure
        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_interface_config iface_config;
        memset(&iface_config, 0, sizeof(iface_config));

        CHECK(roc_receiver_configure(NULL, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                     &iface_config)
              == -1);
        CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT, (roc_interface)-1,
                                     &iface_config)
              == -1);
        CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT,
                                     ROC_INTERFACE_AUDIO_SOURCE, NULL)
              == -1);

        strcpy(iface_config.multicast_group, "1.1.1.256");
        CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT,
                                     ROC_INTERFACE_AUDIO_SOURCE, &iface_config)
              == -1);

        strcpy(iface_config.multicast_group, "2001::eab:dead::a0:abcd:4e");
        CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT,
                                     ROC_INTERFACE_AUDIO_SOURCE, &iface_config)
              == -1);

        strcpy(iface_config.multicast_group, "bad");
        CHECK(roc_receiver_configure(receiver, ROC_SLOT_DEFAULT,
                                     ROC_INTERFACE_AUDIO_SOURCE, &iface_config)
              == -1);

        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // query
        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                source_endpoint)
              == 0);

        roc_receiver_metrics recv_metrics;
        roc_connection_metrics conn_metrics;
        size_t conn_metrics_count = 1;

        // bad
        CHECK(roc_receiver_query(NULL, ROC_SLOT_DEFAULT, &recv_metrics, &conn_metrics,
                                 &conn_metrics_count)
              == -1);
        CHECK(roc_receiver_query(NULL, 999, &recv_metrics, &conn_metrics,
                                 &conn_metrics_count)
              == -1);
        CHECK(roc_receiver_query(receiver, ROC_SLOT_DEFAULT, &recv_metrics, &conn_metrics,
                                 NULL)
              == -1);

        // good
        CHECK(roc_receiver_query(receiver, ROC_SLOT_DEFAULT, &recv_metrics, NULL, NULL)
              == 0);
        CHECK(roc_receiver_query(receiver, ROC_SLOT_DEFAULT, NULL, &conn_metrics,
                                 &conn_metrics_count)
              == 0);
        CHECK(roc_receiver_query(receiver, ROC_SLOT_DEFAULT, &recv_metrics, NULL,
                                 &conn_metrics_count)
              == 0);
        CHECK(roc_receiver_query(receiver, ROC_SLOT_DEFAULT, &recv_metrics, &conn_metrics,
                                 &conn_metrics_count)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // unlink
        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                                source_endpoint)
              == 0);

        CHECK(roc_receiver_unlink(NULL, ROC_SLOT_DEFAULT) == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
}

TEST(receiver, bad_config) {
    { // frame_encoding.rate == 0
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.rate = 0;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // frame_encoding.format == 0
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.format = (roc_format)0;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // frame_encoding.format == 99999
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.format = (roc_format)99999;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // frame_encoding.channels == 0
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.channels = (roc_channel_layout)0;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // frame_encoding.channels == 99999
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.channels = (roc_channel_layout)99999;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // frame_encoding.tracks != 0 (non-multitrack)
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.tracks = 1;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // frame_encoding.tracks == 0 (multitrack)
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MULTITRACK;
        receiver_config_copy.frame_encoding.tracks = 0;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // frame_encoding.tracks == 99999 (multitrack)
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.frame_encoding.channels = ROC_CHANNEL_LAYOUT_MULTITRACK;
        receiver_config_copy.frame_encoding.tracks = 99999;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // clock_source == 99999
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.clock_source = (roc_clock_source)99999;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // resampler_backend == 99999
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.resampler_backend = (roc_resampler_backend)99999;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
    { // resampler_profile == 99999
        roc_receiver_config receiver_config_copy = receiver_config;
        receiver_config_copy.resampler_profile = (roc_resampler_profile)99999;

        roc_receiver* receiver = NULL;
        CHECK(roc_receiver_open(context, &receiver_config_copy, &receiver) != 0);
        CHECK(!receiver);
    }
}

TEST(receiver, read_args) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

    float samples[16] = {};

    { // all good, not bound
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_read(receiver, &frame) == 0);
    }

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

    CHECK(roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                            source_endpoint)
          == 0);

    { // all good, bound
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_read(receiver, &frame) == 0);
    }

    { // null receiver
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_read(NULL, &frame) == -1);
    }

    { // null frame
        CHECK(roc_receiver_read(receiver, NULL) == -1);
    }

    { // null samples, zero sample count
        roc_frame frame;
        frame.samples = NULL;
        frame.samples_size = 0;
        CHECK(roc_receiver_read(receiver, &frame) == 0);
    }

    { // null samples, non-zero sample count
        roc_frame frame;
        frame.samples = NULL;
        frame.samples_size = ROC_ARRAY_SIZE(samples);
        CHECK(roc_receiver_read(receiver, &frame) == -1);
    }

    { // uneven sample count
        roc_frame frame;
        frame.samples = samples;
        frame.samples_size = 1;
        CHECK(roc_receiver_read(receiver, &frame) == -1);
    }

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

} // namespace api
} // namespace roc
