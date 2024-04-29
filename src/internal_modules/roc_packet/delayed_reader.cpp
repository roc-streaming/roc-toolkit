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
#include "roc_core/time.h"
#include "roc_status/code_to_str.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

DelayedReader::DelayedReader(IReader& reader,
                             core::nanoseconds_t target_delay,
                             const audio::SampleSpec& sample_spec)
    : reader_(reader)
    , queue_(0)
    , delay_(0)
    , started_(false)
    , sample_spec_(sample_spec)
    , valid_(false) {
    if (target_delay > 0) {
        delay_ = sample_spec.ns_2_stream_timestamp(target_delay);
    }

    roc_log(LogDebug, "delayed reader: initializing: delay=%lu(%.3fms)",
            (unsigned long)delay_, sample_spec_.stream_timestamp_2_ms(delay_));

    valid_ = true;
}

bool DelayedReader::is_valid() const {
    return valid_;
}

status::StatusCode DelayedReader::read(PacketPtr& ptr) {
    roc_panic_if(!valid_);

    if (!started_) {
        const status::StatusCode code = fetch_packets_();
        if (code != status::StatusOK) {
            return code;
        }

        started_ = true;
    }

    if (queue_.size() != 0) {
        return read_queued_packet_(ptr);
    }

    return reader_.read(ptr);
}

status::StatusCode DelayedReader::fetch_packets_() {
    PacketPtr pp;
    for (;;) {
        status::StatusCode code = reader_.read(pp);
        if (code != status::StatusOK) {
            if (code == status::StatusDrain) {
                break;
            }
            return code;
        }

        code = queue_.write(pp);
        // TODO(gh-183): forward status
        roc_panic_if(code != status::StatusOK);
    }

    const stream_timestamp_t qs = queue_size_();
    if (qs < delay_) {
        return status::StatusDrain;
    }

    roc_log(LogDebug,
            "delayed reader: initial queue:"
            " delay=%lu(%.3fms) queue=%lu(%.3fms) packets=%lu",
            (unsigned long)delay_, sample_spec_.stream_timestamp_2_ms(delay_),
            (unsigned long)qs, sample_spec_.stream_timestamp_2_ms(qs),
            (unsigned long)queue_.size());

    return status::StatusOK;
}

status::StatusCode DelayedReader::read_queued_packet_(PacketPtr& pp) {
    stream_timestamp_t qs = 0;

    for (;;) {
        const status::StatusCode code = queue_.read(pp);
        if (code != status::StatusOK) {
            return code;
        }

        const stream_timestamp_t new_qs = queue_size_();
        if (new_qs < delay_) {
            break;
        }

        qs = new_qs;
    }

    if (qs != 0) {
        roc_log(LogDebug,
                "delayed reader: trimmed queue:"
                " delay=%lu(%.3fms) queue=%lu(%.3fms) packets=%lu",
                (unsigned long)delay_, sample_spec_.stream_timestamp_2_ms(delay_),
                (unsigned long)qs, sample_spec_.stream_timestamp_2_ms(qs),
                (unsigned long)(queue_.size() + 1));
    }

    return status::StatusOK;
}

stream_timestamp_t DelayedReader::queue_size_() const {
    if (queue_.size() == 0) {
        return 0;
    }

    const stream_timestamp_diff_t qs = stream_timestamp_diff(
        queue_.tail()->stream_timestamp() + queue_.tail()->duration(),
        queue_.head()->stream_timestamp());

    if (qs < 0) {
        roc_log(LogError, "delayed reader: unexpected negative queue size: %ld",
                (long)qs);
        return 0;
    }

    return (stream_timestamp_t)qs;
}

} // namespace packet
} // namespace roc
