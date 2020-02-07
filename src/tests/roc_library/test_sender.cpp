/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/stddefs.h"

#include "roc/sender.h"

namespace roc {

TEST_GROUP(sender) {
    roc_sender_config sender_config;

    roc_context* context;

    void setup() {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &context) == 0);
        CHECK(context);

        memset(&sender_config, 0, sizeof(sender_config));
        sender_config.fec_code = ROC_FEC_DISABLE;
        sender_config.frame_sample_rate = 44100;
        sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
        sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;
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

    CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);

    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, connect_errors) {
    { // resolve error
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://invalid.:123") == 0);

        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // connect twice
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // reconnect after error
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);

        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://invalid.:123") == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // connect incomplete endpoint
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);

        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(roc_endpoint_set_host(source_endpoint, "127.0.0.1") == 0);
        CHECK(roc_endpoint_set_port(source_endpoint, 123) == 0);

        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // connect partially invalidated endpoint
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

        // invalidate protocol field
        CHECK(roc_endpoint_set_protocol(source_endpoint, (roc_protocol)-1) == -1);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        // fix protocol field
        CHECK(roc_endpoint_set_protocol(source_endpoint, ROC_PROTO_RTP) == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
}

TEST(sender, outgoing_address) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

    CHECK(roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, "127.0.0.1")
          == 0);
    CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, outgoing_address_errors) {
    { // bad outgoing address
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

        CHECK(
            roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, "8.8.8.8")
            == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(
            roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, "0.0.0.0")
            == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // bad IP family
        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

        CHECK(roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, "::")
              == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);

        CHECK(
            roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, "0.0.0.0")
            == 0);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == 0);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
}

TEST(sender, broadcast_flag) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

    CHECK(roc_sender_set_broadcast_enabled(sender, ROC_INTERFACE_AUDIO_SOURCE, 1) == 0);
    CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, squashing_flag) {
    roc_sender* sender = NULL;
    CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

    roc_endpoint* source_endpoint = NULL;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:123") == 0);

    CHECK(roc_sender_set_squashing_enabled(sender, ROC_INTERFACE_AUDIO_SOURCE, 0) == 0);
    CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint) == 0);

    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    LONGS_EQUAL(0, roc_sender_close(sender));
}

TEST(sender, bad_args) {
    roc_sender* sender = NULL;

    { // open
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
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        roc_endpoint* source_endpoint = NULL;
        CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
        CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:0") == 0);

        CHECK(roc_sender_connect(NULL, ROC_INTERFACE_AUDIO_SOURCE, source_endpoint)
              == -1);
        CHECK(roc_sender_connect(sender, (roc_interface)-1, source_endpoint) == -1);
        CHECK(roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, NULL) == -1);

        CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // set outgoing address
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        CHECK(roc_sender_set_outgoing_address(NULL, ROC_INTERFACE_AUDIO_SOURCE, "0.0.0.0")
              == -1);
        CHECK(roc_sender_set_outgoing_address(sender, (roc_interface)-1, "0.0.0.0")
              == -1);
        CHECK(roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, NULL)
              == -1);

        CHECK(roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE,
                                              "1.1.1.256")
              == -1);
        CHECK(roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE,
                                              "2001::eab:dead::a0:abcd:4e")
              == -1);
        CHECK(roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, "bad")
              == -1);
        CHECK(roc_sender_set_outgoing_address(sender, ROC_INTERFACE_AUDIO_SOURCE, "")
              == -1);

        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // set broadcast flag
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        CHECK(roc_sender_set_broadcast_enabled(NULL, ROC_INTERFACE_AUDIO_SOURCE, 0)
              == -1);
        CHECK(roc_sender_set_broadcast_enabled(sender, (roc_interface)-1, 0) == -1);

        CHECK(roc_sender_set_broadcast_enabled(sender, ROC_INTERFACE_AUDIO_SOURCE, -1)
              == -1);
        CHECK(roc_sender_set_broadcast_enabled(sender, ROC_INTERFACE_AUDIO_SOURCE, 2)
              == -1);

        LONGS_EQUAL(0, roc_sender_close(sender));
    }
    { // set squashing flag
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);

        CHECK(roc_sender_set_squashing_enabled(NULL, ROC_INTERFACE_AUDIO_SOURCE, 0)
              == -1);
        CHECK(roc_sender_set_squashing_enabled(sender, (roc_interface)-1, 0) == -1);

        CHECK(roc_sender_set_squashing_enabled(sender, ROC_INTERFACE_AUDIO_SOURCE, -1)
              == -1);
        CHECK(roc_sender_set_squashing_enabled(sender, ROC_INTERFACE_AUDIO_SOURCE, 2)
              == -1);

        LONGS_EQUAL(0, roc_sender_close(sender));
    }
}

} // namespace roc
