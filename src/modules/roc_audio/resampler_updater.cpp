/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_updater.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogRate = 5000000000;

} // namespace

ResamplerUpdater::ResamplerUpdater(packet::timestamp_t update_interval,
                                   packet::timestamp_t aim_queue_size)
    : writer_(NULL)
    , reader_(NULL)
    , resampler_(NULL)
    , fe_(aim_queue_size)
    , rate_limiter_(LogRate)
    , update_interval_(update_interval)
    , update_time_(0)
    , start_time_(0)
    , has_first_(false)
    , first_(0)
    , has_last_(false)
    , last_(0)
    , started_(false) {
}

void ResamplerUpdater::set_writer(packet::IWriter& writer) {
    roc_panic_if(writer_);
    writer_ = &writer;
}

void ResamplerUpdater::set_reader(packet::IReader& reader) {
    roc_panic_if(reader_);
    reader_ = &reader;
}

void ResamplerUpdater::set_resampler(Resampler& resampler) {
    roc_panic_if(resampler_);
    resampler_ = &resampler;
}

void ResamplerUpdater::write(const packet::PacketPtr& pp) {
    if (!has_last_ || ROC_UNSIGNED_LE(packet::signed_timestamp_t, last_, pp->end())) {
        last_ = pp->end();
        has_last_ = true;
    }
    roc_panic_if(!writer_);
    writer_->write(pp);
}

packet::PacketPtr ResamplerUpdater::read() {
    roc_panic_if(!reader_);
    packet::PacketPtr pp = reader_->read();
    if (!has_first_ && pp) {
        first_ = pp->begin();
        has_first_ = true;
    }
    return pp;
}

bool ResamplerUpdater::update(packet::timestamp_t time) {
    if (!has_first_ || !has_last_) {
        return true;
    }

    if (!started_) {
        started_ = true;
        start_time_ = time;
        update_time_ = time;
    }

    const packet::timestamp_t local_pos = time - start_time_;
    const packet::timestamp_t remote_pos = last_ - first_;

    packet::signed_timestamp_t queue_size =
        ROC_UNSIGNED_SUB(packet::signed_timestamp_t, remote_pos, local_pos);

    if (queue_size < 0) {
        queue_size = 0;
    }

    while (time >= update_time_) {
        fe_.update((packet::timestamp_t)queue_size);
        update_time_ += update_interval_;
    }

    if (rate_limiter_.allow()) {
        roc_log(LogDebug, "resampler updater: local=%lu remote=%lu queue=%lu fe=%.5f",
                (unsigned long)local_pos, (unsigned long)remote_pos,
                (unsigned long)queue_size, (double)fe_.freq_coeff());
    }

    roc_panic_if(!resampler_);
    return resampler_->set_scaling(fe_.freq_coeff());
}

} // namespace audio
} // namespace roc
