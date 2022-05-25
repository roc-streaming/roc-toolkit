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
#include "roc_core/heap_allocator.h"

namespace roc {
namespace address {

namespace {

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(endpoint_uri) {};

TEST(endpoint_uri, empty) {
    EndpointUri u(allocator);

    CHECK(!u.check(EndpointUri::Subset_Full));

    LONGS_EQUAL(Proto_None, u.proto());
    STRCMP_EQUAL("", u.host());
    LONGS_EQUAL(-1, u.port());
    CHECK(!u.path());
    CHECK(!u.encoded_query());

    STRCMP_EQUAL("<bad>", endpoint_uri_to_str(u).c_str());
}

TEST(endpoint_uri, fields) {
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(-1, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/path", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/path", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/path?query", EndpointUri::Subset_Full,
                                 u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        STRCMP_EQUAL("query", u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/path?query", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123?query", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        STRCMP_EQUAL("query", u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123?query", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/?", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123/", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123?", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

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
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtp://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtp+rs8m://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP_RS8M_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp+rs8m://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rs8m://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RS8M_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rs8m://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtp+ldpc://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTP_LDPC_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtp+ldpc://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("ldpc://host:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_LDPC_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("ldpc://host:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, addresses) {
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://[::1]:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("[::1]", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());

        STRCMP_EQUAL("rtsp://[::1]:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, omit_port) {
    EndpointUri u(allocator);

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

TEST(endpoint_uri, service) {
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("123", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointUri u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1", EndpointUri::Subset_Full, u));
        CHECK(u.check(EndpointUri::Subset_Full));

        LONGS_EQUAL(Proto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(-1, u.port());
        STRCMP_EQUAL("554", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, non_empty_path) {
    EndpointUri u(allocator);

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
    EndpointUri u(allocator);
    CHECK(parse_endpoint_uri("rtsp://"
                             "foo-bar"
                             ":123"
                             "/foo%21bar%40baz%2Fqux%3Fwee"
                             "?foo%21bar",
                             EndpointUri::Subset_Full, u));
    CHECK(u.check(EndpointUri::Subset_Full));

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
    EndpointUri u(allocator);
    CHECK(parse_endpoint_uri("rtsp://host:123/path?query", EndpointUri::Subset_Full, u));

    char buf[sizeof("rtsp://host:123/path?query")];

    {
        core::StringBuilder b(buf, sizeof(buf));

        CHECK(format_endpoint_uri(u, EndpointUri::Subset_Full, b));
        CHECK(b.ok());
    }

    for (size_t i = 0; i < sizeof(buf); i++) {
        core::StringBuilder b(buf, i);

        CHECK(format_endpoint_uri(u, EndpointUri::Subset_Full, b));
        CHECK(!b.ok());
    }
}

TEST(endpoint_uri, bad_syntax) {
    EndpointUri u(allocator);

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
