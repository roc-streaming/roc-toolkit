/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/stddefs.h"

#include "roc/context.h"
#include "roc/receiver.h"
#include "roc/sender.h"

namespace roc {
namespace api {

TEST_GROUP(context) {};

TEST(context, open_close) {
    roc_context_config config;
    memset(&config, 0, sizeof(config));

    roc_context* context = NULL;
    CHECK(roc_context_open(&config, &context) == 0);
    CHECK(context);

    LONGS_EQUAL(0, roc_context_close(context));
}

TEST(context, open_null) {
    roc_context* context = NULL;
    LONGS_EQUAL(-1, roc_context_open(NULL, &context));
    CHECK(!context);

    roc_context_config config;
    memset(&config, 0, sizeof(config));
    LONGS_EQUAL(-1, roc_context_open(&config, NULL));
}

TEST(context, close_null) {
    LONGS_EQUAL(-1, roc_context_close(NULL));
}

TEST(context, reference_counting) {
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    roc_context* context = NULL;
    CHECK(roc_context_open(&context_config, &context) == 0);
    CHECK(context);

    {
        roc_sender_config sender_config;
        memset(&sender_config, 0, sizeof(sender_config));
        sender_config.frame_encoding.format = ROC_FORMAT_PCM;
        sender_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
        sender_config.frame_encoding.rate = 44100;
        sender_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
        sender_config.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;

        roc_sender* sender = NULL;
        CHECK(roc_sender_open(context, &sender_config, &sender) == 0);
        CHECK(sender);

        LONGS_EQUAL(-1, roc_context_close(context));

        {
            roc_receiver_config receiver_config;
            memset(&receiver_config, 0, sizeof(receiver_config));
            receiver_config.frame_encoding.format = ROC_FORMAT_PCM;
            receiver_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
            receiver_config.frame_encoding.rate = 44100;
            receiver_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
            roc_receiver* receiver = NULL;
            CHECK(roc_receiver_open(context, &receiver_config, &receiver) == 0);
            CHECK(receiver);

            LONGS_EQUAL(-1, roc_context_close(context));

            LONGS_EQUAL(0, roc_receiver_close(receiver));
        }

        LONGS_EQUAL(-1, roc_context_close(context));

        LONGS_EQUAL(0, roc_sender_close(sender));
    }

    LONGS_EQUAL(0, roc_context_close(context));
}

} // namespace api
} // namespace roc
