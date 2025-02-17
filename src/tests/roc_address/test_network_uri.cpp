/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/network_uri.h"
#include "roc_address/network_uri_to_str.h"
#include "roc_core/heap_arena.h"

namespace roc {
namespace address {

namespace {

core::HeapArena arena;

} // namespace

TEST_GROUP(network_uri) {};

TEST(network_uri, empty) {
    NetworkUri u(arena);

    CHECK(!u.verify(NetworkUri::Subset_Full));

    LONGS_EQUAL(Proto_None, u.proto());
    STRCMP_EQUAL("", u.host());
    LONGS_EQUAL(-1, u.port());
    CHECK(!u.path());
    CHECK(!u.encoded_query());

    STRCMP_EQUAL("<bad>", network_uri_to_str(u).c_str());
}

TEST(network_uri, fields) {
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(-1, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host:123/path", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/path", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host:123/", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(
            parse_network_uri("rtsp://host:123/path?query", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        STRCMP_EQUAL("query", u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/path?query", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host:123?query", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        STRCMP_EQUAL("query", u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123?query", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host:123/?", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host:123?", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", network_uri_to_str(u).c_str());
    }
}

TEST(network_uri, protocols) {
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtp://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp://host:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtp+rs8m://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP_RS8M_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp+rs8m://host:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rs8m://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RS8M_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rs8m://host:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtp+ldpc://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP_LDPC_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp+ldpc://host:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("ldpc://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_LDPC_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("ldpc://host:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtcp://host:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTCP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtcp://host:123", network_uri_to_str(u).c_str());
    }
}

TEST(network_uri, addresses) {
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://127.0.0.1:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://[::1]:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("[::1]", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://[::1]:123", network_uri_to_str(u).c_str());
    }
}

TEST(network_uri, assign) {
    NetworkUri u1(arena);
    NetworkUri u2(arena);

    CHECK(parse_network_uri("rtsp://127.0.0.1:123/path?query", NetworkUri::Subset_Full,
                            u1));
    CHECK(u1.verify(NetworkUri::Subset_Full));

    CHECK(u2.assign(u1));

    LONGS_EQUAL(Proto_RTSP, u2.proto());
    STRCMP_EQUAL("127.0.0.1", u2.host());
    LONGS_EQUAL(123, u2.port());
    STRCMP_EQUAL("/path", u2.path());
    STRCMP_EQUAL("query", u2.encoded_query());

    STRCMP_EQUAL("rtsp://127.0.0.1:123/path?query", network_uri_to_str(u2).c_str());
}

TEST(network_uri, is_equal) {
    NetworkUri a1(arena);
    NetworkUri a2(arena);
    CHECK(parse_network_uri("rtsp://127.0.0.1:123/path?query", NetworkUri::Subset_Full,
                            a1));
    CHECK(parse_network_uri("rtsp://127.0.0.1:123/path?query", NetworkUri::Subset_Full,
                            a2));

    NetworkUri b1(arena);
    NetworkUri b2(arena);
    NetworkUri b3(arena);
    NetworkUri b4(arena);
    CHECK(parse_network_uri("rtsp://127.0.0.2:123/path?query", NetworkUri::Subset_Full,
                            b1));
    CHECK(parse_network_uri("rtsp://127.0.0.1:124/path?query", NetworkUri::Subset_Full,
                            b2));
    CHECK(parse_network_uri("rtsp://127.0.0.1:123/patH?query", NetworkUri::Subset_Full,
                            b3));
    CHECK(parse_network_uri("rtsp://127.0.0.1:123/path?querY", NetworkUri::Subset_Full,
                            b4));

    NetworkUri c1(arena);
    NetworkUri c2(arena);
    NetworkUri c3(arena);
    NetworkUri c4(arena);
    CHECK(parse_network_uri("rtp://127.0.0.1:123", NetworkUri::Subset_Full, c1));
    CHECK(parse_network_uri("rtsp://127.0.0.1/path?query", NetworkUri::Subset_Full, c2));
    CHECK(parse_network_uri("rtsp://127.0.0.1:123/?query", NetworkUri::Subset_Full, c3));
    CHECK(parse_network_uri("rtsp://127.0.0.1:123/path", NetworkUri::Subset_Full, c4));

    CHECK(a1.is_equal(a2));
    CHECK(a2.is_equal(a1));

    CHECK(!a1.is_equal(b1));
    CHECK(!a1.is_equal(b2));
    CHECK(!a1.is_equal(b3));
    CHECK(!a1.is_equal(b4));

    CHECK(!b1.is_equal(a1));
    CHECK(!b2.is_equal(a1));
    CHECK(!b3.is_equal(a1));
    CHECK(!b4.is_equal(a1));

    CHECK(!a1.is_equal(c1));
    CHECK(!a1.is_equal(c2));
    CHECK(!a1.is_equal(c3));
    CHECK(!a1.is_equal(c4));

    CHECK(!c1.is_equal(a1));
    CHECK(!c2.is_equal(a1));
    CHECK(!c3.is_equal(a1));
    CHECK(!c4.is_equal(a1));
}

TEST(network_uri, omit_port) {
    NetworkUri u(arena);

    CHECK(parse_network_uri("rtsp://host:123", NetworkUri::Subset_Full, u));
    CHECK(parse_network_uri("rtsp://host", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rtp://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp://host", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rtp+rs8m://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp+rs8m://host", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rs8m://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rs8m://host", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rtp+ldpc://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp+ldpc://host", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("ldpc://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("ldpc://host", NetworkUri::Subset_Full, u));
}

TEST(network_uri, zero_port) {
    NetworkUri u(arena);
    CHECK(parse_network_uri("rtsp://host:0", NetworkUri::Subset_Full, u));
    CHECK(u.verify(NetworkUri::Subset_Full));

    LONGS_EQUAL(Proto_RTSP, u.proto());
    STRCMP_EQUAL("host", u.host());
    LONGS_EQUAL(0, u.port());
    CHECK(!u.path());
    CHECK(!u.encoded_query());

    STRCMP_EQUAL("rtsp://host:0", network_uri_to_str(u).c_str());
}

TEST(network_uri, service) {
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://127.0.0.1:123", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("123", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", network_uri_to_str(u).c_str());
    }
    {
        NetworkUri u(arena);
        CHECK(parse_network_uri("rtsp://127.0.0.1", NetworkUri::Subset_Full, u));
        CHECK(u.verify(NetworkUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(-1, u.port());
        STRCMP_EQUAL("554", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1", network_uri_to_str(u).c_str());
    }
}

TEST(network_uri, non_empty_path) {
    NetworkUri u(arena);

    CHECK(parse_network_uri("rtsp://host:123", NetworkUri::Subset_Full, u));
    CHECK(parse_network_uri("rtsp://host:123/path", NetworkUri::Subset_Full, u));
    CHECK(parse_network_uri("rtsp://host:123?query", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rtp://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp://host:123/path", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp://host:123?query", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rtp+rs8m://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp+rs8m://host:123/path", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp+rs8m://host:123?query", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rs8m://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rs8m://host:123/path", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rs8m://host:123?query", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("rtp+ldpc://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp+ldpc://host:123/path", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp+ldpc://host:123?query", NetworkUri::Subset_Full, u));

    CHECK(parse_network_uri("ldpc://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("ldpc://host:123/path", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("ldpc://host:123?query", NetworkUri::Subset_Full, u));
}

TEST(network_uri, percent_encoding) {
    NetworkUri u(arena);
    CHECK(parse_network_uri("rtsp://"
                            "foo-bar"
                            ":123"
                            "/foo%21bar%40baz%2Fqux%3Fwee"
                            "?foo%21bar",
                            NetworkUri::Subset_Full, u));
    CHECK(u.verify(NetworkUri::Subset_Full));

    LONGS_EQUAL(Proto_RTSP, u.proto());
    STRCMP_EQUAL("foo-bar", u.host());
    LONGS_EQUAL(123, u.port());
    STRCMP_EQUAL("/foo!bar@baz/qux?wee", u.path());
    STRCMP_EQUAL("foo%21bar", u.encoded_query());

    STRCMP_EQUAL("rtsp://"
                 "foo-bar"
                 ":123"
                 "/foo!bar@baz/qux%3Fwee"
                 "?foo%21bar",
                 network_uri_to_str(u).c_str());
}

TEST(network_uri, small_buffer) {
    NetworkUri u(arena);
    CHECK(parse_network_uri("rtsp://host:123/path?query", NetworkUri::Subset_Full, u));

    char buf[sizeof("rtsp://host:123/path?query")];

    {
        core::StringBuilder b(buf, sizeof(buf));

        CHECK(format_network_uri(u, NetworkUri::Subset_Full, b));
        CHECK(b.is_ok());
    }

    for (size_t i = 0; i < sizeof(buf); i++) {
        core::StringBuilder b(buf, i);

        CHECK(format_network_uri(u, NetworkUri::Subset_Full, b));
        CHECK(!b.is_ok());
    }
}

TEST(network_uri, bad_syntax) {
    NetworkUri u(arena);

    CHECK(parse_network_uri("rtsp://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("bad://host:123", NetworkUri::Subset_Full, u));

    CHECK(!parse_network_uri("host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri(" rtsp://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtp ://host:123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host: 123", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:123 ", NetworkUri::Subset_Full, u));

    CHECK(!parse_network_uri("rtsp://host:port", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:-1", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:65536", NetworkUri::Subset_Full, u));

    CHECK(!parse_network_uri("rtsp://host:123path", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:123./path", NetworkUri::Subset_Full, u));

    CHECK(!parse_network_uri("rtsp://host:123/path%", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:123/path%--path", NetworkUri::Subset_Full, u));

    CHECK(!parse_network_uri("rtsp://host:123/path?query#frag", NetworkUri::Subset_Full,
                             u));
    CHECK(!parse_network_uri("rtsp://host:123/path?#frag", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:123/path#frag", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:123/#frag", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:123#frag", NetworkUri::Subset_Full, u));
    CHECK(!parse_network_uri("rtsp://host:123#", NetworkUri::Subset_Full, u));

    CHECK(!parse_network_uri("", NetworkUri::Subset_Full, u));
}

} // namespace address
} // namespace roc
