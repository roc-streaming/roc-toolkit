/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/heap_arena.h"
#include "roc_sdp/parser.h"

namespace roc {
namespace sdp {

namespace {

core::HeapArena arena;

} // namespace

TEST_GROUP(sdp_parser) {};

TEST(sdp_parser, guid_and_connection) {
    SessionDescription session_description(arena);
    CHECK(parse_sdp("v=0\r\n"
                    "o=test_origin 16914 1 IN IP4 192.168.58.15\r\n"
                    "c=IN IP4 230.123.12.42/250\r\n"
                    "m=audio 12345 RTP/AVP 10",
                    session_description));

    STRCMP_EQUAL("test_origin 16914 IN 192.168.58.15", session_description.guid());

    STRCMP_EQUAL("230.123.12.42:0",
                 address::socket_addr_to_str(
                     session_description.session_connection_data().connection_address())
                     .c_str());
}

TEST(sdp_parser, media_descriptions) {
    SessionDescription session_description(arena);
    CHECK(parse_sdp("v=0\r\n"
                    "o=test_origin 16914 1 IN IP4 192.168.58.15\r\n"
                    "c=IN IP4 230.255.12.42/250\r\n"
                    "m=audio 12345 RTP/AVP 10 11\r\n"
                    "m=video 6789 RTP/AVP 10\r\n"
                    "c=IN IP4 232.111.12.42/250\r\n"
                    "c=IN IP4 232.222.12.42/110\r\n"
                    "m=audio 8787 RTP/AVP 11",
                    session_description));

    core::SharedPtr<MediaDescription> media1 =
        session_description.first_media_description();

    CHECK_EQUAL(2, media1->nb_payload_ids());
    CHECK_EQUAL(11, media1->payload_id(1));

    core::SharedPtr<MediaDescription> media2 =
        session_description.nextof_media_description(media1);
    ConnectionData c1 = media2->connection_data(0);
    ConnectionData c2 = media2->connection_data(media2->nb_connection_data() - 1);

    CHECK_EQUAL(6789, media2->port());
    CHECK_EQUAL(MediaType_Video, media2->type());
    CHECK_EQUAL(MediaTransport_RTP_AVP, media2->transport());
    CHECK_EQUAL(MediaTransport_RTP_AVP, media2->transport());

    STRCMP_EQUAL("232.111.12.42:0",
                 address::socket_addr_to_str(c1.connection_address()).c_str());
    STRCMP_EQUAL("232.222.12.42:0",
                 address::socket_addr_to_str(c2.connection_address()).c_str());

    core::SharedPtr<MediaDescription> media3 =
        session_description.nextof_media_description(media2);
    CHECK_EQUAL(8787, media3->port());
    CHECK_EQUAL(11, media3->default_payload_id());
}

} // namespace sdp
} // namespace roc
