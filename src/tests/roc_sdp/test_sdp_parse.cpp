/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_sdp/session_description.h"

namespace roc {
namespace sdp {

namespace {

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(sdp_parser) {};

TEST(sdp_parser, str) {
    SessionDescription session_description(allocator);
    CHECK(parse_sdp("v=0\r\n"
                    "o=test_origin 16914 1 IN IP4 192.168.58.15\r\n"
                    "c=IN IP4 230.255.12.42/250\r\n"
                    "m=audio 12345 RTP/AVP 10 11\r\n"
                    "m=audio 6789 RTP/AVP 10\r\n"
                    "c=IN IP4 231.255.12.42/250\r\n"
                    "c=IN IP4 232.255.12.42/250",
                    session_description));

    STRCMP_EQUAL("test_origin 16914 IN 192.168.58.15", session_description.guid());

    MediaDescription* last_media = session_description.last_media_description().get();

    CHECK_EQUAL(6789, last_media->port());
    CHECK_EQUAL(MediaType_Audio, last_media->type());
    CHECK_EQUAL(MediaTransport_RTP_AVP, last_media->proto());
    CHECK_EQUAL(10, last_media->default_payload_id());
}

} // namespace sdp
} // namespace roc
