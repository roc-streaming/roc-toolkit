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
    , delay_queue_(0)
    , delay_(0)
    , loaded_(false)
    , unloaded_(false)
    , sample_spec_(sample_spec)
    , init_status_(status::NoStatus) {
    if (target_delay > 0) {
        delay_ = sample_spec.ns_2_stream_timestamp(target_delay);
    }

    roc_log(LogDebug, "delayed reader: initializing: delay=%lu(%.3fms)",
            (unsigned long)delay_, sample_spec_.stream_timestamp_2_ms(delay_));

    init_status_ = status::StatusOK;
}

status::StatusCode DelayedReader::init_status() const {
    return init_status_;
}

status::StatusCode DelayedReader::read(PacketPtr& packet, PacketReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!loaded_) {
        const status::StatusCode code = load_queue_();
        if (code != status::StatusOK) {
            return code;
        }
        loaded_ = true;
    }

    if (!unloaded_) {
        if (delay_queue_.size() != 0) {
            return delay_queue_.read(packet, mode);
        }
        unloaded_ = true;
    }

    return reader_.read(packet, mode);
}

status::StatusCode DelayedReader::load_queue_() {
    // fetch all available packets into queue
    PacketPtr pp;
    for (;;) {
        status::StatusCode code = status::NoStatus;

        if ((code = reader_.read(pp, ModeFetch)) != status::StatusOK) {
            if (code == status::StatusDrain) {
                break;
            }
            return code;
        }

        if ((code = delay_queue_.write(pp)) != status::StatusOK) {
            return code;
        }
    }

    stream_timestamp_t init_qs = calc_queue_duration_();
    if (init_qs < delay_) {
        // return drain until queue is large enough
        return status::StatusDrain;
    }

    // trim queue if it's too big
    stream_timestamp_t trim_qs = init_qs;
    size_t n_dropped = 0;

    while (trim_qs > delay_) {
        const status::StatusCode code = delay_queue_.read(pp, ModeFetch);
        if (code != status::StatusOK) {
            if (code == status::StatusDrain) {
                break;
            }
            return code;
        }
        trim_qs = calc_queue_duration_();
        n_dropped++;
    }

    roc_log(LogNote,
            "delayed reader: starting:"
            " delay=%lu(%.3fms) init_qs=%lu(%.3fms) trim_qs=%lu(%.3fms)"
            " n_drop=%lu n_keep=%lu",
            (unsigned long)delay_, sample_spec_.stream_timestamp_2_ms(delay_),
            (unsigned long)init_qs, sample_spec_.stream_timestamp_2_ms(init_qs),
            (unsigned long)trim_qs, sample_spec_.stream_timestamp_2_ms(trim_qs),
            (unsigned long)n_dropped, (unsigned long)delay_queue_.size());

    return status::StatusOK;
}

stream_timestamp_t DelayedReader::calc_queue_duration_() const {
    if (delay_queue_.size() == 0) {
        return 0;
    }

    const stream_timestamp_diff_t qs = stream_timestamp_diff(
        delay_queue_.tail()->stream_timestamp() + delay_queue_.tail()->duration(),
        delay_queue_.head()->stream_timestamp());

    if (qs < 0) {
        roc_log(LogError, "delayed reader: unexpected negative queue size: %ld",
                (long)qs);
        return 0;
    }

    return (stream_timestamp_t)qs;
}

} // namespace packet
} // namespace roc
