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

    CHECK(!u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123/path", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123/", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123/path?query", EndpointURI::Subset_Full,
                                 u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(
            parse_endpoint_uri("rtsp://host:123/path#frag", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123/path?query#frag",
                                 EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123/?#", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123?query", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123#frag", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123?query#frag", EndpointURI::Subset_Full,
                                 u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123?#", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://host:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtp://host:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtp+rs8m://host:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rs8m://host:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtp+ldpc://host:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("ldpc://host:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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
        CHECK(parse_endpoint_uri("rtsp://[::1]:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

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

    CHECK(parse_endpoint_uri("rtsp://host:123", EndpointURI::Subset_Full, u));
    CHECK(parse_endpoint_uri("rtsp://host", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp://host", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+rs8m://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rs8m://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rs8m://host", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+ldpc://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("ldpc://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("ldpc://host", EndpointURI::Subset_Full, u));
}

TEST(endpoint_uri, service) {
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1:123", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(123, u.port());
        STRCMP_EQUAL("123", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1:123", endpoint_uri_to_str(u).c_str());
    }
    {
        EndpointURI u(allocator);
        CHECK(parse_endpoint_uri("rtsp://127.0.0.1", EndpointURI::Subset_Full, u));
        CHECK(u.check(EndpointURI::Subset_Full));

        LONGS_EQUAL(EndProto_RTSP, u.proto());
        STRCMP_EQUAL("127.0.0.1", u.host());
        LONGS_EQUAL(-1, u.port());
        STRCMP_EQUAL("554", u.service());

        STRCMP_EQUAL("rtsp://127.0.0.1", endpoint_uri_to_str(u).c_str());
    }
}

TEST(endpoint_uri, non_empty_path) {
    EndpointURI u(allocator);

    CHECK(parse_endpoint_uri("rtsp://host:123", EndpointURI::Subset_Full, u));
    CHECK(parse_endpoint_uri("rtsp://host:123/path", EndpointURI::Subset_Full, u));
    CHECK(parse_endpoint_uri("rtsp://host:123?query", EndpointURI::Subset_Full, u));
    CHECK(parse_endpoint_uri("rtsp://host:123#frag", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp://host:123/path", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp://host:123?query", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp://host:123#frag", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+rs8m://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123/path", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123?query", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+rs8m://host:123#frag", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rs8m://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rs8m://host:123/path", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rs8m://host:123?query", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rs8m://host:123#frag", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("rtp+ldpc://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123/path", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123?query", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp+ldpc://host:123#frag", EndpointURI::Subset_Full, u));

    CHECK(parse_endpoint_uri("ldpc://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("ldpc://host:123/path", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("ldpc://host:123?query", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("ldpc://host:123#frag", EndpointURI::Subset_Full, u));
}

TEST(endpoint_uri, percent_encoding) {
    EndpointURI u(allocator);
    CHECK(parse_endpoint_uri("rtsp://"
                             "foo-bar"
                             ":123"
                             "/foo%21bar%40baz%2Fqux%3Fwee"
                             "?foo%21bar"
                             "#foo%21bar",
                             EndpointURI::Subset_Full, u));
    CHECK(u.check(EndpointURI::Subset_Full));

    LONGS_EQUAL(EndProto_RTSP, u.proto());
    STRCMP_EQUAL("foo-bar", u.host());
    LONGS_EQUAL(123, u.port());
    STRCMP_EQUAL("/foo!bar@baz/qux?wee", u.path());
    STRCMP_EQUAL("foo%21bar", u.encoded_query());
    STRCMP_EQUAL("foo%21bar", u.encoded_fragment());

    STRCMP_EQUAL("rtsp://"
                 "foo-bar"
                 ":123"
                 "/foo!bar@baz/qux%3Fwee"
                 "?foo%21bar"
                 "#foo%21bar",
                 endpoint_uri_to_str(u).c_str());
}

TEST(endpoint_uri, small_buffer) {
    EndpointURI u(allocator);
    CHECK(parse_endpoint_uri("rtsp://host:123/path?query#frag", EndpointURI::Subset_Full,
                             u));

    char buf[32];

    {
        core::StringBuilder b(buf, sizeof(buf));

        CHECK(format_endpoint_uri(u, EndpointURI::Subset_Full, b));
        CHECK(b.ok());
    }

    for (size_t i = 0; i < sizeof(buf); i++) {
        core::StringBuilder b(buf, i);

        CHECK(format_endpoint_uri(u, EndpointURI::Subset_Full, b));
        CHECK(!b.ok());
    }
}

TEST(endpoint_uri, bad_syntax) {
    EndpointURI u(allocator);

    CHECK(parse_endpoint_uri("rtsp://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("bad://host:123", EndpointURI::Subset_Full, u));

    CHECK(!parse_endpoint_uri("host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri(" rtsp://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtp ://host:123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host: 123", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123 ", EndpointURI::Subset_Full, u));

    CHECK(!parse_endpoint_uri("rtsp://host:port", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:-1", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:65536", EndpointURI::Subset_Full, u));

    CHECK(!parse_endpoint_uri("rtsp://host:123path", EndpointURI::Subset_Full, u));
    CHECK(!parse_endpoint_uri("rtsp://host:123./path", EndpointURI::Subset_Full, u));

    CHECK(!parse_endpoint_uri("rtsp://host:123/path%", EndpointURI::Subset_Full, u));
    CHECK(
        !parse_endpoint_uri("rtsp://host:123/path%--path", EndpointURI::Subset_Full, u));

    CHECK(!parse_endpoint_uri("", EndpointURI::Subset_Full, u));
}

} // namespace address
} // namespace roc
