/*
 * Copyright (c) 2019 Roc authors
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

TEST_GROUP(endpoint_uri) {
    core::HeapAllocator allocator;
};

TEST(endpoint_uri, empty) {
    EndpointURI u(allocator);

    CHECK(!u.is_valid());

    LONGS_EQUAL(EndProto_None, u.proto());
    STRCMP_EQUAL("", u.host());
    LONGS_EQUAL(-1, u.port());
    CHECK(!u.path());
    CHECK(!u.encoded_query());
    CHECK(!u.encoded_fragment());

    STRCMP_EQUAL("<bad>", endpoint_uri_to_str(u).c_str());
}

TEST(endpoint_uri, fields) {
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(-1, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/path", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123/path", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123/", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/path?query", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        STRCMP_EQUAL("query", u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123/path?query", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/path#frag", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        CHECK(!u.encoded_query());
        STRCMP_EQUAL("frag", u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123/path#frag", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/path?query#frag", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/path", u.path());
        STRCMP_EQUAL("query", u.encoded_query());
        STRCMP_EQUAL("frag", u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123/path?query#frag", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123/?#", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("/", u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123/", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123?query", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        STRCMP_EQUAL("query", u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123?query", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123#frag", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        STRCMP_EQUAL("frag", u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123#frag", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123?query#frag", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        STRCMP_EQUAL("query", u.encoded_query());
        STRCMP_EQUAL("frag", u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123?query#frag", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123?#", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, protocols) {
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://host:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtp://host:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTP, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtp://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtp+rs8m://host:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTP_RS8M_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtp+rs8m://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rs8m://host:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RS8M_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rs8m://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtp+ldpc://host:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTP_LDPC_Source, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtp+ldpc://host:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("ldpc://host:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_LDPC_Repair, u.proto());
        STRCMP_EQUAL("host", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("ldpc://host:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, addresses) {
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://[::1]:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("[::1]", u.host());
        LONGS_EQUAL(123, u.port());
        CHECK(!u.path());
        CHECK(!u.encoded_query());
        CHECK(!u.encoded_fragment());

        STRCMP_EQUAL("rtsp://[::1]:123", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, omit_port) {
    EndpointURI u(allocator);

    CHECK(parse_endpoint_uri("rtsp://host:123", u));
    CHECK(parse_endpoint_uri("rtsp://host", u));

    CHECK(parse_endpoint_uri("rtp://host:123", u));
    CHECK(!parse_endpoint_uri("rtp://host", u));

    CHECK(parse_endpoint_uri("rtp+rs8m://host:123", u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host", u));

    CHECK(parse_endpoint_uri("rs8m://host:123", u));
    CHECK(!parse_endpoint_uri("rs8m://host", u));

    CHECK(parse_endpoint_uri("rtp+ldpc://host:123", u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host", u));

    CHECK(parse_endpoint_uri("ldpc://host:123", u));
    CHECK(!parse_endpoint_uri("ldpc://host", u));
}

TEST(endpoint_uri, service) {
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("123", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1", u));
        CHECK(u.is_valid());

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(-1, u.port());
        STRCMP_EQUAL("554", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, non_empty_path) {
    EndpointURI u(allocator);

    CHECK(parse_endpoint_uri("rtsp://host:123", u));
    CHECK(parse_endpoint_uri("rtsp://host:123/path", u));
    CHECK(parse_endpoint_uri("rtsp://host:123?query", u));
    CHECK(parse_endpoint_uri("rtsp://host:123#frag", u));

    CHECK(parse_endpoint_uri("rtp://host:123", u));
    CHECK(!parse_endpoint_uri("rtp://host:123/path", u));
    CHECK(!parse_endpoint_uri("rtp://host:123?query", u));
    CHECK(!parse_endpoint_uri("rtp://host:123#frag", u));

    CHECK(parse_endpoint_uri("rtp+rs8m://host:123", u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123/path", u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123?query", u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123#frag", u));

    CHECK(parse_endpoint_uri("rs8m://host:123", u));
    CHECK(!parse_endpoint_uri("rs8m://host:123/path", u));
    CHECK(!parse_endpoint_uri("rs8m://host:123?query", u));
    CHECK(!parse_endpoint_uri("rs8m://host:123#frag", u));

    CHECK(parse_endpoint_uri("rtp+ldpc://host:123", u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123/path", u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123?query", u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123#frag", u));

    CHECK(parse_endpoint_uri("ldpc://host:123", u));
    CHECK(!parse_endpoint_uri("ldpc://host:123/path", u));
    CHECK(!parse_endpoint_uri("ldpc://host:123?query", u));
    CHECK(!parse_endpoint_uri("ldpc://host:123#frag", u));
}

TEST(endpoint_uri, percent_encoding) {
    EndpointURI u(allocator);
    CHECK(parse_endpoint_uri("rtsp://"
                             "foo%21bar%40baz%2Fqux%3Fwee"
                             ":123"
                             "/foo%21bar%40baz%2Fqux%3Fwee"
                             "?foo%21bar"
                             "#foo%21bar",
                             u));
    CHECK(u.is_valid());

    LONGS_EQUAL(EndProto_RTSP, u.proto());
    STRCMP_EQUAL("foo!bar@baz/qux?wee", u.host());
    LONGS_EQUAL(123, u.port());
    STRCMP_EQUAL("/foo!bar@baz/qux?wee", u.path());
    STRCMP_EQUAL("foo%21bar", u.encoded_query());
    STRCMP_EQUAL("foo%21bar", u.encoded_fragment());

    STRCMP_EQUAL("rtsp://"
                 "foo!bar%40baz%2Fqux%3Fwee"
                 ":123"
                 "/foo!bar@baz/qux%3Fwee"
                 "?foo%21bar"
                 "#foo%21bar",
                 endpoint_uri_to_str(u).c_str());
}

TEST(endpoint_uri, small_buffer) {
    EndpointURI u(allocator);
    CHECK(parse_endpoint_uri("rtsp://host:123/path?query#frag", u));

    char buf[32];
    CHECK(format_endpoint_uri(u, buf, sizeof(buf)));

    for (size_t i = 0; i < sizeof(buf); i++) {
        CHECK(!format_endpoint_uri(u, buf, i));
    }
}

TEST(endpoint_uri, bad_syntax) {
    EndpointURI u(allocator);

    CHECK(parse_endpoint_uri("rtsp://host:123", u));
    CHECK(!parse_endpoint_uri("bad://host:123", u));

    CHECK(!parse_endpoint_uri("host:123", u));
    CHECK(!parse_endpoint_uri("://host:123", u));
    CHECK(!parse_endpoint_uri("rtsp://", u));
    CHECK(!parse_endpoint_uri("rtsp://:123", u));
    CHECK(!parse_endpoint_uri(" rtsp://host:123", u));
    CHECK(!parse_endpoint_uri("rtp ://host:123", u));
    CHECK(!parse_endpoint_uri("rtsp://host: 123", u));
    CHECK(!parse_endpoint_uri("rtsp://host:123 ", u));

    CHECK(!parse_endpoint_uri("rtsp://host:port", u));
    CHECK(!parse_endpoint_uri("rtsp://host:-1", u));
    CHECK(!parse_endpoint_uri("rtsp://host:65536", u));

    CHECK(!parse_endpoint_uri("rtsp://host:123path", u));
    CHECK(!parse_endpoint_uri("rtsp://host:123./path", u));

    CHECK(!parse_endpoint_uri("rtsp://host%:123/path", u));
    CHECK(!parse_endpoint_uri("rtsp://host%--host:123/path", u));
    CHECK(!parse_endpoint_uri("rtsp://host:123/path%", u));
    CHECK(!parse_endpoint_uri("rtsp://host:123/path%--path", u));

    CHECK(!parse_endpoint_uri("", u));
}

} // namespace address
} // namespace roc
