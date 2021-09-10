/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/stddefs.h"

#include "roc/endpoint.h"

namespace roc {

TEST_GROUP(endpoint) {};

TEST(endpoint, alloc_dealloc) {
    roc_endpoint* endp = NULL;

    CHECK(roc_endpoint_allocate(&endp) == 0);
    CHECK(endp != NULL);

    CHECK(roc_endpoint_deallocate(endp) == 0);
}

TEST(endpoint, uri_string) {
    roc_endpoint* endp = NULL;
    CHECK(roc_endpoint_allocate(&endp) == 0);

    CHECK(roc_endpoint_set_uri(endp, "rtsp://host:123/path?query") == 0);

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("rtsp://host:123/path?query", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    {
        roc_protocol proto;
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        UNSIGNED_LONGS_EQUAL(ROC_PROTO_RTSP, proto);
    }

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("host", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    {
        int port;
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        UNSIGNED_LONGS_EQUAL(123, port);
    }

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("/path?query", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    CHECK(roc_endpoint_deallocate(endp) == 0);
}

TEST(endpoint, uri_parts) {
    roc_endpoint* endp = NULL;
    CHECK(roc_endpoint_allocate(&endp) == 0);

    CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);
    CHECK(roc_endpoint_set_host(endp, "host") == 0);
    CHECK(roc_endpoint_set_port(endp, 123) == 0);
    CHECK(roc_endpoint_set_resource(endp, "/path?query") == 0);

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("rtsp://host:123/path?query", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    {
        roc_protocol proto;
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        UNSIGNED_LONGS_EQUAL(ROC_PROTO_RTSP, proto);
    }

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("host", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    {
        int port;
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        UNSIGNED_LONGS_EQUAL(123, port);
    }

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("/path?query", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    CHECK(roc_endpoint_deallocate(endp) == 0);
}

TEST(endpoint, override_uri_parts) {
    roc_endpoint* endp = NULL;
    CHECK(roc_endpoint_allocate(&endp) == 0);

    CHECK(roc_endpoint_set_uri(endp, "rtsp://host:123/path?query") == 0);

    CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTP) == 0);
    CHECK(roc_endpoint_set_host(endp, "1.2.3.4") == 0);
    CHECK(roc_endpoint_set_port(endp, 567) == 0);
    CHECK(roc_endpoint_set_resource(endp, "") == 0);

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("rtp://1.2.3.4:567", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    {
        roc_protocol proto;
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        UNSIGNED_LONGS_EQUAL(ROC_PROTO_RTP, proto);
    }

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        STRCMP_EQUAL("1.2.3.4", buf);
        UNSIGNED_LONGS_EQUAL(strlen(buf) + 1, bufsz);
    }

    {
        int port;
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        UNSIGNED_LONGS_EQUAL(567, port);
    }

    {
        char buf[128];
        size_t bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) != 0);
    }

    CHECK(roc_endpoint_deallocate(endp) == 0);
}

TEST(endpoint, missing_parts) {
    char buf[128];
    size_t bufsz;
    roc_protocol proto;
    int port;

    { // missing proto
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        CHECK(roc_endpoint_set_host(endp, "1.2.3.4") == 0);
        CHECK(roc_endpoint_set_port(endp, 567) == 0);
        CHECK(roc_endpoint_set_resource(endp, "/path") == 0);

        // proto: no
        CHECK(roc_endpoint_get_protocol(endp, &proto) == -1);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // missing host
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);
        CHECK(roc_endpoint_set_port(endp, 567) == 0);
        CHECK(roc_endpoint_set_resource(endp, "/path") == 0);

        // proto: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        // host: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == -1);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // missing port (uri parts)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);
        CHECK(roc_endpoint_set_host(endp, "1.2.3.4") == 0);
        CHECK(roc_endpoint_set_resource(endp, "/path") == 0);

        // proto: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // missing port (uri string)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4/path") == 0);

        // proto: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // missing resource (uri parts)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);
        CHECK(roc_endpoint_set_host(endp, "1.2.3.4") == 0);
        CHECK(roc_endpoint_set_port(endp, 567) == 0);

        // proto: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // resource: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // missing resource (uri string)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567") == 0);

        // proto: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // resource: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
}

TEST(endpoint, clear_parts) {
    char buf[128];
    size_t bufsz;
    int port;

    { // clear port
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri with port
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // clear port
        CHECK(roc_endpoint_set_port(endp, -1) == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // clear resource (NULL)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri with resource
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // clear resource
        CHECK(roc_endpoint_set_resource(endp, NULL) == 0);

        // resource: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // clear resource ("")
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri with resource
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // clear resource
        CHECK(roc_endpoint_set_resource(endp, "") == 0);

        // resource: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
}

TEST(endpoint, invalidate_parts) {
    char buf[128];
    size_t bufsz;
    roc_protocol proto;
    int port;

    { // invalidate protocol
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // protocol: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        // invalidate protocol
        CHECK(roc_endpoint_set_protocol(endp, (roc_protocol)-1) == -1);

        // protocol: no
        CHECK(roc_endpoint_get_protocol(endp, &proto) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        // restore protocol
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);

        // protocol: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // invalidate host (NULL)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        // invalidate host
        CHECK(roc_endpoint_set_host(endp, NULL) == -1);

        // host: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        // restore host
        CHECK(roc_endpoint_set_host(endp, "1.2.3.4") == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // invalidate host ("")
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        // invalidate host
        CHECK(roc_endpoint_set_host(endp, "") == -1);

        // host: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        // restore host
        CHECK(roc_endpoint_set_host(endp, "1.2.3.4") == 0);

        // host: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // invalidate port (positive)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        // invalidate port
        CHECK(roc_endpoint_set_port(endp, 100000) == -1);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        // restore port
        CHECK(roc_endpoint_set_port(endp, 567) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // invalidate port (negative)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        // invalidate port
        CHECK(roc_endpoint_set_port(endp, -1000) == -1);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        // restore port
        CHECK(roc_endpoint_set_port(endp, 567) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // invalidate resource
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path?query") == 0);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        // invalidate resource
        CHECK(roc_endpoint_set_resource(endp, "BAD") == -1);

        // resource: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        // restore resource
        CHECK(roc_endpoint_set_resource(endp, "/new") == 0);

        // resource: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("/new", buf);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // invalidate uri
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // protocol, host, port, resource: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);
        CHECK(roc_endpoint_get_port(endp, &port) == 0);
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        // invalidate uri
        CHECK(roc_endpoint_set_uri(endp, "BAD") == -1);

        // protocol, host, port, resource: no
        CHECK(roc_endpoint_get_protocol(endp, &proto) == -1);
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == -1);
        CHECK(roc_endpoint_get_port(endp, &port) == -1);
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        // restore uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://1.2.3.4:567/path") == 0);

        // protocol, host, port, resource: yes
        CHECK(roc_endpoint_get_protocol(endp, &proto) == 0);
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);
        CHECK(roc_endpoint_get_port(endp, &port) == 0);
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
}

TEST(endpoint, standard_port) {
    char buf[128];
    size_t bufsz;
    int port;

    { // set uri without port (protocol defines standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtsp://host") == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtsp://host", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set uri without port (protocol doesn't define standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp, "rtp://host") == -1);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set protocol, don't set port (protocol defines standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);

        // set protocol
        CHECK(roc_endpoint_set_host(endp, "host") == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtsp://host", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set protocol, don't set port (protocol doesn't define standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);

        // set protocol
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTP) == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set protocol, then set port (protocol defines standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);

        // set protocol
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);

        // set port
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtsp://host:123", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set protocol, then set port (protocol doesn't define standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);

        // set protocol
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTP) == 0);

        // set port
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtp://host:123", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set port, don't set protocol
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);

        // set port
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set port, then set protocol (protocol defines standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);

        // set port
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // set protocol
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtsp://host:123", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set port, then set protocol (protocol doesn't define standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);

        // set port
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // set protocol
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTP) == 0);

        // port: yes
        CHECK(roc_endpoint_get_port(endp, &port) == 0);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtp://host:123", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // clear port (protocol defines standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set protocol, host, port
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // clear port
        CHECK(roc_endpoint_set_port(endp, -1) == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: yes
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtsp://host", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // clear port (protocol doesn't define standard port)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set protocol, host, port
        CHECK(roc_endpoint_set_protocol(endp, ROC_PROTO_RTP) == 0);
        CHECK(roc_endpoint_set_host(endp, "host") == 0);
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // clear port
        CHECK(roc_endpoint_set_port(endp, -1) == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // clear port (protocol not set)
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set host, port
        CHECK(roc_endpoint_set_host(endp, "host") == 0);
        CHECK(roc_endpoint_set_port(endp, 123) == 0);

        // clear port
        CHECK(roc_endpoint_set_port(endp, -1) == 0);

        // port: no
        CHECK(roc_endpoint_get_port(endp, &port) == -1);

        // uri: no
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
}

TEST(endpoint, percent_encoding) {
    char buf[128];
    size_t bufsz;

    { // set uri
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set uri
        CHECK(roc_endpoint_set_uri(endp,
                                   "rtsp://"
                                   "foo-bar"
                                   ":123"
                                   "/foo%21bar%40baz%2Fqux%3Fwee"
                                   "?foo%21bar")
              == 0);

        // get uri
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("rtsp://"
                     "foo-bar"
                     ":123"
                     "/foo!bar@baz/qux%3Fwee"
                     "?foo%21bar",
                     buf);

        // get resource
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("/foo!bar@baz/qux%3Fwee"
                     "?foo%21bar",
                     buf);

        // get host
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("foo-bar", buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
    { // set resource
        roc_endpoint* endp = NULL;
        CHECK(roc_endpoint_allocate(&endp) == 0);

        // set resource
        CHECK(roc_endpoint_set_resource(endp,
                                        "/foo%21bar%40baz%2Fqux%3Fwee"
                                        "?foo%21bar")
              == 0);

        // get resource
        bufsz = sizeof(buf);
        CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == 0);
        STRCMP_EQUAL("/foo!bar@baz/qux%3Fwee"
                     "?foo%21bar",
                     buf);

        CHECK(roc_endpoint_deallocate(endp) == 0);
    }
}

TEST(endpoint, null_buffer) {
    roc_endpoint* endp = NULL;
    CHECK(roc_endpoint_allocate(&endp) == 0);
    CHECK(roc_endpoint_set_uri(endp, "rtsp://host:123/path?query") == 0);

    size_t bufsz;

    // uri
    bufsz = 0;
    CHECK(roc_endpoint_get_uri(endp, NULL, &bufsz) == 0);
    UNSIGNED_LONGS_EQUAL(strlen("rtsp://host:123/path?query") + 1, bufsz);

    // resource
    bufsz = 0;
    CHECK(roc_endpoint_get_resource(endp, NULL, &bufsz) == 0);
    UNSIGNED_LONGS_EQUAL(strlen("/path?query") + 1, bufsz);

    // host
    bufsz = 0;
    CHECK(roc_endpoint_get_host(endp, NULL, &bufsz) == 0);
    UNSIGNED_LONGS_EQUAL(strlen("host") + 1, bufsz);

    CHECK(roc_endpoint_deallocate(endp) == 0);
}

TEST(endpoint, bad_args_alloc_dealloc) {
    CHECK(roc_endpoint_allocate(NULL) == -1);
    CHECK(roc_endpoint_deallocate(NULL) == -1);
}

TEST(endpoint, bad_args_set) {
    roc_endpoint* endp = NULL;
    CHECK(roc_endpoint_allocate(&endp) == 0);

    // uri: not ok
    CHECK(roc_endpoint_set_uri(NULL, "rtsp://host") == -1);
    CHECK(roc_endpoint_set_uri(endp, NULL) == -1);
    CHECK(roc_endpoint_set_uri(endp, "BAD") == -1);

    // protocol: not ok
    CHECK(roc_endpoint_set_protocol(NULL, ROC_PROTO_RTP) == -1);
    CHECK(roc_endpoint_set_protocol(endp, (roc_protocol)-1) == -1);

    // host: not ok
    CHECK(roc_endpoint_set_host(NULL, NULL) == -1);
    CHECK(roc_endpoint_set_host(endp, NULL) == -1);
    CHECK(roc_endpoint_set_host(endp, "") == -1);

    // port: ok
    CHECK(roc_endpoint_set_port(endp, -1) == 0);
    CHECK(roc_endpoint_set_port(endp, 0) == 0);
    CHECK(roc_endpoint_set_port(endp, 1) == 0);
    CHECK(roc_endpoint_set_port(endp, 65535) == 0);

    // port: not ok
    CHECK(roc_endpoint_set_port(NULL, 0) == -1);
    CHECK(roc_endpoint_set_port(endp, -2) == -1);
    CHECK(roc_endpoint_set_port(endp, 65536) == -1);

    // resource: ok
    CHECK(roc_endpoint_set_resource(endp, "/path") == 0);
    CHECK(roc_endpoint_set_resource(endp, NULL) == 0);
    CHECK(roc_endpoint_set_resource(endp, "") == 0);

    // resource: not ok
    CHECK(roc_endpoint_set_resource(NULL, "/path") == -1);
    CHECK(roc_endpoint_set_resource(endp, "BAD") == -1);

    CHECK(roc_endpoint_deallocate(endp) == 0);
}

TEST(endpoint, bad_args_get) {
    char buf[128];
    size_t bufsz;
    roc_protocol proto;
    int port;

    roc_endpoint* endp = NULL;
    CHECK(roc_endpoint_allocate(&endp) == 0);
    CHECK(roc_endpoint_set_uri(endp, "rtsp://host:123/path") == 0);

    // uri: not ok
    bufsz = sizeof(buf);
    CHECK(roc_endpoint_get_uri(NULL, buf, &bufsz) == -1);

    bufsz = 0;
    CHECK(roc_endpoint_get_uri(endp, buf, NULL) == -1);

    bufsz = 0;
    CHECK(roc_endpoint_get_uri(endp, buf, &bufsz) == -1);

    // protocol: not ok
    CHECK(roc_endpoint_get_protocol(NULL, &proto) == -1);
    CHECK(roc_endpoint_get_protocol(endp, NULL) == -1);

    // host: not ok
    bufsz = sizeof(buf);
    CHECK(roc_endpoint_get_host(NULL, buf, &bufsz) == -1);

    bufsz = 0;
    CHECK(roc_endpoint_get_host(endp, buf, NULL) == -1);

    bufsz = 0;
    CHECK(roc_endpoint_get_host(endp, buf, &bufsz) == -1);

    // port: not ok
    CHECK(roc_endpoint_get_port(NULL, &port) == -1);
    CHECK(roc_endpoint_get_port(endp, NULL) == -1);

    // resource: not ok
    bufsz = sizeof(buf);
    CHECK(roc_endpoint_get_resource(NULL, buf, &bufsz) == -1);

    bufsz = 0;
    CHECK(roc_endpoint_get_resource(endp, buf, NULL) == -1);

    bufsz = 0;
    CHECK(roc_endpoint_get_resource(endp, buf, &bufsz) == -1);

    CHECK(roc_endpoint_deallocate(endp) == 0);
}

} // namespace roc
