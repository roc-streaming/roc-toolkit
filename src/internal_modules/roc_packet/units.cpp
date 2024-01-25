/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/units.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

stream_timestamp_t ns_2_stream_timestamp(const core::nanoseconds_t ns,
                                         const size_t sample_rate) {
    roc_panic_if_msg(ns < 0, "units: ns should not be negative");
    roc_panic_if_msg(sample_rate == 0, "units: sample_rate should not be zero");

    float ts = roundf(float(ns) / core::Second * sample_rate);

    ts = std::min(ts, (float)ROC_MAX_OF(stream_timestamp_t));
    ts = std::max(ts, (float)ROC_MIN_OF(stream_timestamp_t));

    return (stream_timestamp_t)ts;
}

core::nanoseconds_t stream_timestamp_2_ns(stream_timestamp_t ts,
                                          const size_t sample_rate) {
    roc_panic_if_msg(sample_rate == 0, "units: sample_rate should not be zero");

    return (core::nanoseconds_t)roundf(float(ts) / sample_rate * core::Second);
}

stream_timestamp_diff_t ns_2_stream_timestamp_delta(const core::nanoseconds_t ns,
                                                    const size_t sample_rate) {
    roc_panic_if_msg(sample_rate == 0, "units: sample_rate should not be zero");

    float ts = roundf(float(ns) / core::Second * sample_rate);

    ts = std::min(ts, (float)ROC_MAX_OF(stream_timestamp_diff_t));
    ts = std::max(ts, (float)ROC_MIN_OF(stream_timestamp_diff_t));

    return (stream_timestamp_diff_t)ts;
}

core::nanoseconds_t stream_timestamp_delta_2_ns(stream_timestamp_diff_t ts,
                                                const size_t sample_rate) {
    roc_panic_if_msg(sample_rate == 0, "units: sample_rate should not be zero");

    return (core::nanoseconds_t)roundf(float(ts) / sample_rate * core::Second);
}

} // namespace packet
} // namespace roc
