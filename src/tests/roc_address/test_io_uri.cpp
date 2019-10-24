/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/io_uri.h"
#include "roc_address/io_uri_to_str.h"

namespace roc {
namespace address {

TEST_GROUP(io_uri) {};

TEST(io_uri, empty) {
    IoURI u;

    CHECK(!u.is_valid());
    CHECK(!u.is_file());
    CHECK(!u.is_special_file());

    STRCMP_EQUAL("", u.scheme());
    STRCMP_EQUAL("", u.path());

    STRCMP_EQUAL("<bad>", io_uri_to_str(u).c_str());
}

TEST(io_uri, device) {
    IoURI u;
    CHECK(parse_io_uri("alsa://card0/subcard1", u));

    CHECK(u.is_valid());
    CHECK(!u.is_file());
    CHECK(!u.is_special_file());

    STRCMP_EQUAL("alsa", u.scheme());
    STRCMP_EQUAL("card0/subcard1", u.path());

    STRCMP_EQUAL("alsa://card0/subcard1", io_uri_to_str(u).c_str());
}

TEST(io_uri, file_localhost_abspath) {
    IoURI u;
    CHECK(parse_io_uri("file://localhost/home/user/test.mp3", u));

    CHECK(u.is_valid());
    CHECK(u.is_file());
    CHECK(!u.is_special_file());

    STRCMP_EQUAL("file", u.scheme());
    STRCMP_EQUAL("/home/user/test.mp3", u.path());

    STRCMP_EQUAL("file:/home/user/test.mp3", io_uri_to_str(u).c_str());
}

TEST(io_uri, file_emptyhost_abspath) {
    IoURI u;
    CHECK(parse_io_uri("file:///home/user/test.mp3", u));

    CHECK(u.is_valid());
    CHECK(u.is_file());
    CHECK(!u.is_special_file());

    STRCMP_EQUAL("file", u.scheme());
    STRCMP_EQUAL("/home/user/test.mp3", u.path());

    STRCMP_EQUAL("file:/home/user/test.mp3", io_uri_to_str(u).c_str());
}

TEST(io_uri, file_emptyhost_specialpath) {
    IoURI u;
    CHECK(parse_io_uri("file://-", u));

    CHECK(u.is_valid());
    CHECK(u.is_file());
    CHECK(u.is_special_file());

    STRCMP_EQUAL("file", u.scheme());
    STRCMP_EQUAL("-", u.path());

    STRCMP_EQUAL("file:-", io_uri_to_str(u).c_str());
}

TEST(io_uri, file_compact_abspath) {
    IoURI u;
    CHECK(parse_io_uri("file:/home/user/test.mp3", u));

    CHECK(u.is_valid());
    CHECK(u.is_file());
    CHECK(!u.is_special_file());

    STRCMP_EQUAL("file", u.scheme());
    STRCMP_EQUAL("/home/user/test.mp3", u.path());

    STRCMP_EQUAL("file:/home/user/test.mp3", io_uri_to_str(u).c_str());
}

TEST(io_uri, file_compact_relpath1) {
    IoURI u;
    CHECK(parse_io_uri("file:./test.mp3", u));

    CHECK(u.is_valid());
    CHECK(u.is_file());
    CHECK(!u.is_special_file());

    STRCMP_EQUAL("file", u.scheme());
    STRCMP_EQUAL("./test.mp3", u.path());

    STRCMP_EQUAL("file:./test.mp3", io_uri_to_str(u).c_str());
}

TEST(io_uri, file_compact_relpath2) {
    IoURI u;
    CHECK(parse_io_uri("file:test/test.mp3", u));

    CHECK(u.is_valid());
    CHECK(u.is_file());
    CHECK(!u.is_special_file());

    STRCMP_EQUAL("file", u.scheme());
    STRCMP_EQUAL("test/test.mp3", u.path());

    STRCMP_EQUAL("file:test/test.mp3", io_uri_to_str(u).c_str());
}

TEST(io_uri, file_compact_specialpath) {
    IoURI u;
    CHECK(parse_io_uri("file:-", u));

    CHECK(u.is_valid());
    CHECK(u.is_file());
    CHECK(u.is_special_file());

    STRCMP_EQUAL("file", u.scheme());
    STRCMP_EQUAL("-", u.path());

    STRCMP_EQUAL("file:-", io_uri_to_str(u).c_str());
}

TEST(io_uri, percent_encoding) {
    {
        IoURI u;
        CHECK(parse_io_uri("alsa://foo%21/bar!%2Fbaz%23", u));

        STRCMP_EQUAL("alsa", u.scheme());
        STRCMP_EQUAL("foo!/bar!/baz#", u.path());

        STRCMP_EQUAL("alsa://foo!/bar!/baz%23", io_uri_to_str(u).c_str());
    }

    {
        IoURI u;
        CHECK(parse_io_uri("file:///foo%21/bar!%2Fbaz%23", u));

        STRCMP_EQUAL("file", u.scheme());
        STRCMP_EQUAL("/foo!/bar!/baz#", u.path());

        STRCMP_EQUAL("file:/foo!/bar!/baz%23", io_uri_to_str(u).c_str());
    }

    {
        IoURI u;
        CHECK(parse_io_uri("file:foo%21/bar!%2Fbaz%23", u));

        STRCMP_EQUAL("file", u.scheme());
        STRCMP_EQUAL("foo!/bar!/baz#", u.path());

        STRCMP_EQUAL("file:foo!/bar!/baz%23", io_uri_to_str(u).c_str());
    }
}

TEST(io_uri, small_buffer) {
    IoURI u;
    CHECK(parse_io_uri("abcdef://abcdef", u));

    char buf[16];
    CHECK(format_io_uri(u, buf, sizeof(buf)));

    for (size_t i = 0; i < sizeof(buf); i++) {
        CHECK(!format_io_uri(u, buf, i));
    }
}

TEST(io_uri, bad_syntax) {
    IoURI u;

    CHECK(parse_io_uri("abcdefg://test", u));
    CHECK(!parse_io_uri("abcdefghijklmnop://test", u));

    CHECK(!parse_io_uri("alsa://", u));
    CHECK(!parse_io_uri("file://", u));
    CHECK(!parse_io_uri("file:/", u));
    CHECK(!parse_io_uri("file:", u));

    CHECK(!parse_io_uri(" alsa://test", u));
    CHECK(!parse_io_uri("alsa ://test", u));
    CHECK(!parse_io_uri("alsa: //test", u));
    CHECK(!parse_io_uri("alsa:/ /test", u));

    CHECK(!parse_io_uri("://test", u));
    CHECK(!parse_io_uri("alsa:/test", u));
    CHECK(!parse_io_uri("alsa::test", u));
    CHECK(!parse_io_uri("alsa//test", u));
    CHECK(!parse_io_uri("alsa/test", u));

    CHECK(!parse_io_uri("file://test", u));
    CHECK(!parse_io_uri("file://./test", u));
    CHECK(!parse_io_uri("file://../test", u));

    CHECK(!parse_io_uri("file://test%", u));
    CHECK(!parse_io_uri("file://test%--test", u));

    CHECK(!parse_io_uri("file://test?test", u));
    CHECK(!parse_io_uri("file://test?test#test", u));
    CHECK(!parse_io_uri("file://test#test", u));
    CHECK(!parse_io_uri("file://?", u));
    CHECK(!parse_io_uri("file://#", u));

    CHECK(!parse_io_uri("test", u));
    CHECK(!parse_io_uri("/test", u));
    CHECK(!parse_io_uri("./test", u));

    CHECK(!parse_io_uri("", u));
}

} // namespace address
} // namespace roc
