/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/ntp.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

TEST_GROUP(headers) {};

TEST(headers, set_bit_field) {
    uint32_t val = 0;

    header::set_bit_field(val, (uint32_t)0xdd, 4, 0xf);
    CHECK_EQUAL(0xd0, val);

    header::set_bit_field(val, (uint32_t)0xc, 8, 0xf);
    CHECK_EQUAL(0xcd0, val);

    header::set_bit_field(val, (uint32_t)0xe, 4, 0xf);
    CHECK_EQUAL(0xce0, val);
}

TEST(headers, extend_timestamp) {
    { // no wrap
        const packet::ntp_timestamp_t base = 0xAAAABBBBCCCCDDDD;
        const packet::ntp_timestamp_t value = 0x0000CCCCDDDD0000;

        CHECK_EQUAL(0xAAAACCCCDDDD0000, header::extend_timestamp(base, value));
    }
    { // wrap
        const packet::ntp_timestamp_t base = 0xAAAABBBBCCCCDDDD;
        const packet::ntp_timestamp_t value = 0x0000111122220000;

        CHECK_EQUAL(0xAAAB111122220000, header::extend_timestamp(base, value));
    }
}

TEST(headers, timestamps) {
    { // SR
        header::SenderReportPacket blk;

        blk.set_ntp_timestamp(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.ntp_timestamp());

        blk.set_ntp_timestamp(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD1111, blk.ntp_timestamp());

        blk.set_ntp_timestamp(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDD8888, blk.ntp_timestamp());

        blk.set_ntp_timestamp(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x1111AABBCCDD0000, blk.ntp_timestamp());
    }
    { // RRTR
        header::XrRrtrBlock blk;

        blk.set_ntp_timestamp(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.ntp_timestamp());

        blk.set_ntp_timestamp(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD1111, blk.ntp_timestamp());

        blk.set_ntp_timestamp(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDD8888, blk.ntp_timestamp());

        blk.set_ntp_timestamp(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x1111AABBCCDD0000, blk.ntp_timestamp());
    }
    { // LSR
        header::ReceptionReportBlock blk;

        blk.set_last_sr(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_sr());

        blk.set_last_sr(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_sr());

        blk.set_last_sr(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_sr());

        blk.set_last_sr(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_sr());
    }
    { // DLSR
        header::ReceptionReportBlock blk;

        blk.set_delay_last_sr(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.delay_last_sr());

        blk.set_delay_last_sr(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.delay_last_sr());

        blk.set_delay_last_sr(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDE0000, blk.delay_last_sr());

        blk.set_delay_last_sr(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.delay_last_sr());
    }
    { // LRR
        header::XrDlrrSubblock blk;

        blk.set_last_rr(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_rr());

        blk.set_last_rr(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_rr());

        blk.set_last_rr(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_rr());

        blk.set_last_rr(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.last_rr());
    }
    { // DLRR
        header::XrDlrrSubblock blk;

        blk.set_delay_last_rr(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.delay_last_rr());

        blk.set_delay_last_rr(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.delay_last_rr());

        blk.set_delay_last_rr(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDE0000, blk.delay_last_rr());

        blk.set_delay_last_rr(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.delay_last_rr());
    }
}

TEST(headers, intervals) {
    { // interval_duration
        header::XrMeasurementInfoBlock blk;

        blk.set_interval_duration(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.interval_duration());

        blk.set_interval_duration(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.interval_duration());

        blk.set_interval_duration(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDE0000, blk.interval_duration());

        blk.set_interval_duration(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.interval_duration());
    }
    { // cumulative_duration
        header::XrMeasurementInfoBlock blk;

        blk.set_cum_duration(0x0000AABBCCDD0000);
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.cum_duration());

        blk.set_cum_duration(0x0000AABBCCDD1111);
        CHECK_EQUAL(0x0000AABBCCDD1111, blk.cum_duration());

        blk.set_cum_duration(0x0000AABBCCDD8888);
        CHECK_EQUAL(0x0000AABBCCDD8888, blk.cum_duration());

        blk.set_cum_duration(0x1111AABBCCDD0000);
        CHECK_EQUAL(0x1111AABBCCDD0000, blk.cum_duration());
    }
}

TEST(headers, metrics) {
    { // mean_rtt
        header::XrDelayMetricsBlock blk;

        CHECK(!blk.has_mean_rtt());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.mean_rtt());

        blk.set_mean_rtt(0x0000AABBCCDD0000);
        CHECK(blk.has_mean_rtt());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.mean_rtt());

        blk.set_mean_rtt(0x0000AABBCCDD1111);
        CHECK(blk.has_mean_rtt());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.mean_rtt());

        blk.set_mean_rtt(0x0000AABBCCDD8888);
        CHECK(blk.has_mean_rtt());
        CHECK_EQUAL(0x0000AABBCCDE0000, blk.mean_rtt());

        blk.set_mean_rtt(0x1111AABBCCDD0000);
        CHECK(blk.has_mean_rtt());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.mean_rtt());

        blk.set_mean_rtt(0x0000FFFFFFFE8000);
        CHECK(blk.has_mean_rtt());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.mean_rtt());

        blk.reset();

        CHECK(!blk.has_mean_rtt());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.mean_rtt());
    }
    { // min_rtt
        header::XrDelayMetricsBlock blk;

        CHECK(!blk.has_min_rtt());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.min_rtt());

        blk.set_min_rtt(0x0000AABBCCDD0000);
        CHECK(blk.has_min_rtt());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.min_rtt());

        blk.set_min_rtt(0x0000AABBCCDD1111);
        CHECK(blk.has_min_rtt());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.min_rtt());

        blk.set_min_rtt(0x0000AABBCCDD8888);
        CHECK(blk.has_min_rtt());
        CHECK_EQUAL(0x0000AABBCCDE0000, blk.min_rtt());

        blk.set_min_rtt(0x1111AABBCCDD0000);
        CHECK(blk.has_min_rtt());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.min_rtt());

        blk.set_min_rtt(0x0000FFFFFFFE8000);
        CHECK(blk.has_min_rtt());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.min_rtt());

        blk.reset();

        CHECK(!blk.has_min_rtt());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.min_rtt());
    }
    { // max_rtt
        header::XrDelayMetricsBlock blk;

        CHECK(!blk.has_max_rtt());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.max_rtt());

        blk.set_max_rtt(0x0000AABBCCDD0000);
        CHECK(blk.has_max_rtt());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.max_rtt());

        blk.set_max_rtt(0x0000AABBCCDD1111);
        CHECK(blk.has_max_rtt());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.max_rtt());

        blk.set_max_rtt(0x0000AABBCCDD8888);
        CHECK(blk.has_max_rtt());
        CHECK_EQUAL(0x0000AABBCCDE0000, blk.max_rtt());

        blk.set_max_rtt(0x1111AABBCCDD0000);
        CHECK(blk.has_max_rtt());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.max_rtt());

        blk.set_max_rtt(0x0000FFFFFFFE8000);
        CHECK(blk.has_max_rtt());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.max_rtt());

        blk.reset();

        CHECK(!blk.has_max_rtt());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.max_rtt());
    }
    { // e2e_delay
        header::XrDelayMetricsBlock blk;

        CHECK(!blk.has_e2e_delay());
        CHECK_EQUAL(0xFFFFFFFFFFFFFFFF, blk.e2e_delay());

        blk.set_e2e_delay(0x0000AABBCCDD0000);
        CHECK(blk.has_e2e_delay());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.e2e_delay());

        blk.set_e2e_delay(0x0000AABBCCDD1111);
        CHECK(blk.has_e2e_delay());
        CHECK_EQUAL(0x0000AABBCCDD1111, blk.e2e_delay());

        blk.set_e2e_delay(0x1111AABBCCDD0000);
        CHECK(blk.has_e2e_delay());
        CHECK_EQUAL(0x1111AABBCCDD0000, blk.e2e_delay());

        blk.set_e2e_delay(0xFFFFFFFFFFFFFFFF);
        CHECK(blk.has_e2e_delay());
        CHECK_EQUAL(0xFFFFFFFFFFFFFFFE, blk.e2e_delay());

        blk.reset();

        CHECK(!blk.has_e2e_delay());
        CHECK_EQUAL(0xFFFFFFFFFFFFFFFF, blk.e2e_delay());
    }
    { // niq_delay
        header::XrQueueMetricsBlock blk;

        CHECK(!blk.has_niq_delay());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.niq_delay());

        blk.set_niq_delay(0x0000AABBCCDD0000);
        CHECK(blk.has_niq_delay());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.niq_delay());

        blk.set_niq_delay(0x0000AABBCCDD1111);
        CHECK(blk.has_niq_delay());
        CHECK_EQUAL(0x0000AABBCCDD0000, blk.niq_delay());

        blk.set_niq_delay(0x0000AABBCCDD8888);
        CHECK(blk.has_niq_delay());
        CHECK_EQUAL(0x0000AABBCCDE0000, blk.niq_delay());

        blk.set_niq_delay(0x1111AABBCCDD0000);
        CHECK(blk.has_niq_delay());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.niq_delay());

        blk.set_niq_delay(0x0000FFFFFFFE8000);
        CHECK(blk.has_niq_delay());
        CHECK_EQUAL(0x0000FFFFFFFE0000, blk.niq_delay());

        blk.reset();

        CHECK(!blk.has_niq_delay());
        CHECK_EQUAL(0x0000FFFFFFFF0000, blk.niq_delay());
    }
}

} // namespace rtcp
} // namespace roc
