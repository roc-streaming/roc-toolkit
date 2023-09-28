/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/delayed_reader.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

DelayedReader::DelayedReader(IReader& reader,
                             core::nanoseconds_t delay,
                             const audio::SampleSpec& sample_spec)
    : reader_(reader)
    , queue_(0)
    , delay_((packet::stream_timestamp_t)sample_spec.ns_2_stream_timestamp_delta(delay))
    , started_(false) {
    roc_log(LogDebug, "delayed reader: initializing: delay=%lu", (unsigned long)delay_);
}

PacketPtr DelayedReader::read() {
    if (!started_) {
        if (!fetch_packets_()) {
            return NULL;
        }

        started_ = true;
    }

    if (queue_.size() != 0) {
        return read_queued_packet_();
    }

    return reader_.read();
}

bool DelayedReader::fetch_packets_() {
    while (PacketPtr pp = reader_.read()) {
        queue_.write(pp);
    }

    const stream_timestamp_t qs = queue_size_();
    if (qs < delay_) {
        return false;
    }

    roc_log(LogDebug, "delayed reader: initial queue: delay=%lu queue=%lu packets=%lu",
            (unsigned long)delay_, (unsigned long)qs, (unsigned long)queue_.size());

    return true;
}

PacketPtr DelayedReader::read_queued_packet_() {
    PacketPtr pp;

    stream_timestamp_t qs = 0;

    for (;;) {
        pp = queue_.read();

        const stream_timestamp_t new_qs = queue_size_();
        if (new_qs < delay_) {
            break;
        }

        qs = new_qs;
    }

    if (qs != 0) {
        roc_log(
            LogDebug, "delayed reader: trimmed queue: delay=%lu queue=%lu packets=%lu",
            (unsigned long)delay_, (unsigned long)qs, (unsigned long)(queue_.size() + 1));
    }

    return pp;
}

stream_timestamp_t DelayedReader::queue_size_() const {
    if (queue_.size() == 0) {
        return 0;
    }

    const stream_timestamp_diff_t qs =
        stream_timestamp_diff(queue_.tail()->end(), queue_.head()->begin());

    if (qs < 0) {
        roc_log(LogError, "delayed reader: unexpected negative queue size: %ld",
                (long)qs);
        return 0;
    }

    return (stream_timestamp_t)qs;
}

} // namespace packet
} // namespace roc
