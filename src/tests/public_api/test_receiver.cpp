/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/stddefs.h"

#include "roc/receiver.h"

namespace roc {

TEST_GROUP(receiver) {
    roc_receiver_config receiver_config;

    roc_context* context;

    void setup() {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &context) == 0);
        CHECK(context);

        memset(&receiver_config, 0, sizeof(receiver_config));
        receiver_config.frame_sample_rate = 44100;
        receiver_config.frame_channels = ROC_CHANNEL_SET_STEREO;
        receiver_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;
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

    CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, bind_errors) {
    roc_receiver* receiver = NULL;

    { // resolve error
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://invalid.:0") == 0);

        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // bind twice
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // rebind after error
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://8.8.8.8:0") == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // bind incomplete endpoint
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);

        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_set_host(source_endpoint, "127.0.0.1") == 0);
        CHECK(roc_endpoint_set_port(source_endpoint, 0) == 0);

        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // bind partially invalidated endpoint
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        // invalidate protocol field
        CHECK(roc_endpoint_set_protocol(source_endpoint, (roc_protocol)-1) == -1);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        // fix protocol field
        CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
}

TEST(receiver, multicast_group) {
    roc_receiver* receiver = NULL;
    CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
    CHECK(receiver);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

    CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(source_endpoint, "224.0.0.1") == 0);
    CHECK(roc_endpoint_set_port(source_endpoint, 0) == 0);

    CHECK(
        roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE, "0.0.0.0")
        == 0);
    CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_receiver_close(receiver));
}

TEST(receiver, multicast_group_errors) {
    roc_receiver* receiver = NULL;

    { // set multicast group but bind to non-multicast address
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE,
                                               "0.0.0.0")
              == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://224.0.0.1:0") == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // bad multicast group address
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://224.0.0.1:0") == 0);

        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE,
                                               "8.8.8.8")
              == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE,
                                               "0.0.0.0")
              == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // bad IP family
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://224.0.0.1:0") == 0);

        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE, "::")
              == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE,
                                               "0.0.0.0")
              == 0);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
}

TEST(receiver, bad_args) {
    roc_receiver* receiver = NULL;

    { // open
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
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        CHECK(roc_receiver_bind(NULL, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint) == -1);
        CHECK(roc_receiver_bind(receiver, (roc_interface)-1, source_endpoint) == -1);
        CHECK(roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, NULL) == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
    { // set multicast group
        CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);

        CHECK(
            roc_receiver_set_multicast_group(NULL, ROC_INTERFACE_AUDIO_SOURCE, "0.0.0.0")
            == -1);
        CHECK(roc_receiver_set_multicast_group(receiver, (roc_interface)-1, "0.0.0.0")
              == -1);
        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE, NULL)
              == -1);

        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE,
                                               "1.1.1.256")
              == -1);
        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE,
                                               "2001::eab:dead::a0:abcd:4e")
              == -1);
        CHECK(
            roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE, "bad")
            == -1);
        CHECK(roc_receiver_set_multicast_group(receiver, ROC_INTERFACE_AUDIO_SOURCE, "")
              == -1);

        LONGS_EQUAL(0, roc_receiver_close(receiver));
    }
}

} // namespace roc
