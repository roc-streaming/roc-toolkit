/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_pipeline/parse_port.h"
#include "roc_pipeline/port_to_str.h"

namespace roc {
namespace pipeline {

TEST_GROUP(port) {};

TEST(port, all_fields) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:123", port));

    UNSIGNED_LONGS_EQUAL(address::EndProto_RTP, port.protocol);
    CHECK(port.address.has_host_port());
    UNSIGNED_LONGS_EQUAL(4, port.address.version());
    LONGS_EQUAL(123, port.address.port());

    STRCMP_EQUAL("rtp:1.2.3.4:123", port_to_str(port).c_str());
}

TEST(port, proto_rtp) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:123", port));

    UNSIGNED_LONGS_EQUAL(address::EndProto_RTP, port.protocol);

    STRCMP_EQUAL("rtp:1.2.3.4:123", port_to_str(port).c_str());
}

TEST(port, proto_rs8m_source) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp+rs8m:1.2.3.4:123", port));

    UNSIGNED_LONGS_EQUAL(address::EndProto_RTP_RS8M_Source, port.protocol);

    STRCMP_EQUAL("rtp+rs8m:1.2.3.4:123", port_to_str(port).c_str());
}

TEST(port, proto_rs8m_repair) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioRepair, "rs8m:1.2.3.4:123", port));

    UNSIGNED_LONGS_EQUAL(address::EndProto_RS8M_Repair, port.protocol);

    STRCMP_EQUAL("rs8m:1.2.3.4:123", port_to_str(port).c_str());
}

TEST(port, proto_ldpc_source) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp+ldpc:1.2.3.4:123", port));

    UNSIGNED_LONGS_EQUAL(address::EndProto_RTP_LDPC_Source, port.protocol);

    STRCMP_EQUAL("rtp+ldpc:1.2.3.4:123", port_to_str(port).c_str());
}

TEST(port, proto_ldpc_repair) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioRepair, "ldpc:1.2.3.4:123", port));

    UNSIGNED_LONGS_EQUAL(address::EndProto_LDPC_Repair, port.protocol);

    STRCMP_EQUAL("ldpc:1.2.3.4:123", port_to_str(port).c_str());
}

TEST(port, addr_zero) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp:0.0.0.0:0", port));

    STRCMP_EQUAL("rtp:0.0.0.0:0", port_to_str(port).c_str());
}

TEST(port, addr_empty) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp::123", port));

    STRCMP_EQUAL("rtp:0.0.0.0:123", port_to_str(port).c_str());
}

TEST(port, addr_ipv4) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp:1.2.0.255:123", port));

    CHECK(port.address.has_host_port());
    UNSIGNED_LONGS_EQUAL(4, port.address.version());

    STRCMP_EQUAL("rtp:1.2.0.255:123", port_to_str(port).c_str());
}

TEST(port, addr_ipv6) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp:[2001:db8::1]:123", port));

    CHECK(port.address.has_host_port());
    UNSIGNED_LONGS_EQUAL(6, port.address.version());

    STRCMP_EQUAL("rtp:[2001:db8::1]:123", port_to_str(port).c_str());
}

TEST(port, port_range_min) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:0", port));

    LONGS_EQUAL(0, port.address.port());

    STRCMP_EQUAL("rtp:1.2.3.4:0", port_to_str(port).c_str());
}

TEST(port, port_range_max) {
    PortConfig port;
    CHECK(parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:65535", port));

    LONGS_EQUAL(65535, port.address.port());

    STRCMP_EQUAL("rtp:1.2.3.4:65535", port_to_str(port).c_str());
}

TEST(port, port_type) {
    PortConfig port;

    CHECK(parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioRepair, "rtp:1.2.3.4:123", port));

    CHECK(parse_port(address::EndType_AudioSource, "rtp+rs8m:1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioRepair, "rtp+rs8m:1.2.3.4:123", port));

    CHECK(!parse_port(address::EndType_AudioSource, "rs8m:1.2.3.4:123", port));
    CHECK(parse_port(address::EndType_AudioRepair, "rs8m:1.2.3.4:123", port));

    CHECK(parse_port(address::EndType_AudioSource, "rtp+ldpc:1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioRepair, "rtp+ldpc:1.2.3.4:123", port));

    CHECK(!parse_port(address::EndType_AudioSource, "ldpc:1.2.3.4:123", port));
    CHECK(parse_port(address::EndType_AudioRepair, "ldpc:1.2.3.4:123", port));
}

TEST(port, bad_format) {
    PortConfig port;
    CHECK(!parse_port(address::EndType_AudioSource, NULL, port));
    CHECK(!parse_port(address::EndType_AudioSource, "", port));
    CHECK(!parse_port(address::EndType_AudioSource, ":", port));
    CHECK(!parse_port(address::EndType_AudioSource, "::", port));
    CHECK(!parse_port(address::EndType_AudioSource, "::::::::::::::::", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4", port));
    CHECK(!parse_port(address::EndType_AudioSource, "1.2.3.4:123", port));
}

TEST(port, bad_protocol) {
    PortConfig port;
    CHECK(!parse_port(address::EndType_AudioSource, " rtp:1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp :1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, ":1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "none:1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rt:1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "RTP:1.2.3.4:123", port));
}

TEST(port, bad_addr) {
    PortConfig port;
    CHECK(!parse_port(address::EndType_AudioSource, "rtp: 1.2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4 :123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1 .2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.a.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.-2.3.4:123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:256.1.2.3:123", port));
}

TEST(port, bad_port_number) {
    PortConfig port;
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4: 123", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:123 ", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:a", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:65536", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:-1", port));
    CHECK(!parse_port(address::EndType_AudioSource, "rtp:1.2.3.4:999999999999999", port));
}

} // namespace pipeline
} // namespace roc
