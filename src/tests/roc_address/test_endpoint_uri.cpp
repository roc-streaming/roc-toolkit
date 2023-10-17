/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/endpoint_uri.h"
#include "roc_address/endpoint_uri_to_str.h"
#include "roc_core/heap_arena.h"

namespace roc {
namespace address {

namespace {

core::HeapArena arena;

} // namespace

TEST_GROUP(endpoint_uri) {};

TEST(endpoint_uri, empty) {
    EndpointUri u(arena);

    CHECK(!u.verify(EndpointUri::Subset_Full));

    LONGS_EQUAL(Proto_None, u.proto());
    STRCMP_EQUAL("", u.host());
    LONGS_EQUAL(-1, u.port());
    CHECK(!u.path());
    CHECK(!u.encoded_query());

    STRCMP_EQUAL("<bad>", endpoint_uri_to_str(u).c_str());
}

TEST(endpoint_uri, fields) {
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(-1, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123/path", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/path", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123/", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123/path?query", EndpointUri::Subset_Full,
                                 u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        STRCMP_EQUAL("query", u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/path?query", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123?query", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        STRCMP_EQUAL("query", u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123?query", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123/?", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123?", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, protocols) {
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtp://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtp+rs8m://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP_RS8M_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp+rs8m://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rs8m://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RS8M_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rs8m://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtp+ldpc://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP_LDPC_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp+ldpc://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("ldpc://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_LDPC_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("ldpc://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtcp://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTCP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtcp://host:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, addresses) {
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://[::1]:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("[::1]", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://[::1]:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, assign) {
    EndpointUri u1(arena);
    EndpointUri u2(arena);

    CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123/path?query", EndpointUri::Subset_Full,
                             u1));
    CHECK(u1.verify(EndpointUri::Subset_Full));

    CHECK(u2.assign(u1));

    LONGS_EQUAL(Proto_RTSP, u2.proto());
    STRCMP_EQUAL("127.0.0.1", u2.host());
    LONGS_EQUAL(123, u2.port());
    STRCMP_EQUAL("/path", u2.path());
    STRCMP_EQUAL("query", u2.encoded_query());

    STRCMP_EQUAL("rtsp://127.0.0.1:123/path?query", endpoint_uri_to_str(u2).c_str());
}

TEST(endpoint_uri, is_equal) {
    EndpointUri a1(arena);
    EndpointUri a2(arena);
    CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123/path?query", EndpointUri::Subset_Full,
                             a1));
    CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123/path?query", EndpointUri::Subset_Full,
                             a2));

    EndpointUri b1(arena);
    EndpointUri b2(arena);
    EndpointUri b3(arena);
    EndpointUri b4(arena);
    CHECK(parse_endpoint_uri("rtsp://127.0.0.2:123/path?query", EndpointUri::Subset_Full,
                             b1));
    CHECK(parse_endpoint_uri("rtsp://127.0.0.1:124/path?query", EndpointUri::Subset_Full,
                             b2));
    CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123/patH?query", EndpointUri::Subset_Full,
                             b3));
    CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123/path?querY", EndpointUri::Subset_Full,
                             b4));

    EndpointUri c1(arena);
    EndpointUri c2(arena);
    EndpointUri c3(arena);
    EndpointUri c4(arena);
    CHECK(parse_endpoint_uri("rtp://127.0.0.1:123", EndpointUri::Subset_Full, c1));
    CHECK(
        parse_endpoint_uri("rtsp://127.0.0.1/path?query", EndpointUri::Subset_Full, c2));
    CHECK(
        parse_endpoint_uri("rtsp://127.0.0.1:123/?query", EndpointUri::Subset_Full, c3));
    CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123/path", EndpointUri::Subset_Full, c4));

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

TEST(endpoint_uri, omit_port) {
    EndpointUri u(arena);

    CHECK(parse_endpoint_uri("rtsp://host:123", EndpointUri::Subset_Full, u));
    CHECK(parse_endpoint_uri("rtsp://host", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp://host", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+rs8m://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rs8m://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rs8m://host", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+ldpc://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("ldpc://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("ldpc://host", EndpointUri::Subset_Full, u));
}

TEST(endpoint_uri, zero_port) {
    EndpointUri u(arena);
    CHECK(parse_endpoint_uri("rtsp://host:0", EndpointUri::Subset_Full, u));
    CHECK(u.verify(EndpointUri::Subset_Full));

    LONGS_EQUAL(Proto_RTSP, u.proto());
    STRCMP_EQUAL("host", u.host());
    LONGS_EQUAL(0, u.port());
    CHECK(!u.path());
    CHECK(!u.encoded_query());

    STRCMP_EQUAL("rtsp://host:0", endpoint_uri_to_str(u).c_str());
}

TEST(endpoint_uri, service) {
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("123", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(arena);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1", EndpointUri::Subset_Full, u));
        CHECK(u.verify(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(-1, u.port());
        STRCMP_EQUAL("554", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, non_empty_path) {
    EndpointUri u(arena);

    CHECK(parse_endpoint_uri("rtsp://host:123", EndpointUri::Subset_Full, u));
    CHECK(parse_endpoint_uri("rtsp://host:123/path", EndpointUri::Subset_Full, u));
    CHECK(parse_endpoint_uri("rtsp://host:123?query", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp://host:123/path", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp://host:123?query", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+rs8m://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123/path", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123?query", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rs8m://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rs8m://host:123/path", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rs8m://host:123?query", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+ldpc://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123/path", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123?query", EndpointUri::Subset_Full, u));

    CHECK(parse_endpoint_uri("ldpc://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("ldpc://host:123/path", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("ldpc://host:123?query", EndpointUri::Subset_Full, u));
}

TEST(endpoint_uri, percent_encoding) {
    EndpointUri u(arena);
    CHECK(parse_endpoint_uri("rtsp://"
                             "foo-bar"
                             ":123"
                             "/foo%21bar%40baz%2Fqux%3Fwee"
                             "?foo%21bar",
                             EndpointUri::Subset_Full, u));
    CHECK(u.verify(EndpointUri::Subset_Full));

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
                 endpoint_uri_to_str(u).c_str());
}

TEST(endpoint_uri, small_buffer) {
    EndpointUri u(arena);
    CHECK(parse_endpoint_uri("rtsp://host:123/path?query", EndpointUri::Subset_Full, u));

    char buf[sizeof("rtsp://host:123/path?query")];

    {
        core::StringBuilder b(buf, sizeof(buf));

        CHECK(format_endpoint_uri(u, EndpointUri::Subset_Full, b));
        CHECK(b.is_ok());
    }

    for (size_t i = 0; i < sizeof(buf); i++) {
        core::StringBuilder b(buf, i);

        CHECK(format_endpoint_uri(u, EndpointUri::Subset_Full, b));
        CHECK(!b.is_ok());
    }
}

TEST(endpoint_uri, bad_syntax) {
    EndpointUri u(arena);

    CHECK(parse_endpoint_uri("rtsp://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("bad://host:123", EndpointUri::Subset_Full, u));

    CHECK(!parse_endpoint_uri("host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri(" rtsp://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp ://host:123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host: 123", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123 ", EndpointUri::Subset_Full, u));

    CHECK(!parse_endpoint_uri("rtsp://host:port", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:-1", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:65536", EndpointUri::Subset_Full, u));

    CHECK(!parse_endpoint_uri("rtsp://host:123path", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123./path", EndpointUri::Subset_Full, u));

    CHECK(!parse_endpoint_uri("rtsp://host:123/path%", EndpointUri::Subset_Full, u));
    CHECK(
        !parse_endpoint_uri("rtsp://host:123/path%--path", EndpointUri::Subset_Full, u));

    CHECK(!parse_endpoint_uri("rtsp://host:123/path?query#frag", EndpointUri::Subset_Full,
                              u));
    CHECK(!parse_endpoint_uri("rtsp://host:123/path?#frag", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123/path#frag", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123/#frag", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123#frag", EndpointUri::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123#", EndpointUri::Subset_Full, u));

    CHECK(!parse_endpoint_uri("", EndpointUri::Subset_Full, u));
}

} // namespace address
} // namespace roc
