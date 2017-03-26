/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_core/helpers.h"

#include "roc_audio/scaler.h"

#define TS_IS_BEFORE(a, b) ROC_IS_BEFORE(packet::signed_timestamp_t, a, b)
#define TS_SUBTRACT(a, b) ROC_SUBTRACT(packet::signed_timestamp_t, a, b)

namespace roc {
namespace audio {

namespace {

enum { ReportInterval = 5000 /* ms */ };

} // namespace

Scaler::Scaler(packet::IPacketReader& reader,
               packet::PacketQueue const& queue,
               packet::timestamp_t aim_queue_size)
    : reader_(reader)
    , queue_(queue)
    , aim_queue_size_(aim_queue_size)
    , freq_estimator_(aim_queue_size)
    , timer_(ReportInterval)
    , started_(false) {
}

packet::IPacketConstPtr Scaler::read() {
    packet::IPacketConstPtr pp = reader_.read();
    update_packet_(head_, pp);
    return pp;
}

bool Scaler::update() {
    update_packet_(tail_, queue_.tail());

    const packet::timestamp_t qs = queue_size_();

    if (!started_) {
        if (qs < aim_queue_size_) {
            return true;
        } else {
            started_ = true;
            roc_log(LogInfo, "scaler: received enough samples:"
                               " queue_size=%lu aim_queue_size=%lu",
                    (unsigned long)qs, (unsigned long)aim_queue_size_);
        }
    }

    freq_estimator_.update(qs);

    const float fc = freq_estimator_.freq_coeff();

    if (timer_.expired()) {
        roc_log(LogDebug, "scaler: queue_size=%05lu freq_coeff=%.5lf", (unsigned long)qs,
                (double)fc);
    }

    for (size_t n = 0; n < resamplers_.size(); n++) {
        if (!resamplers_[n]->set_scaling(fc)) {
            return false;
        }
    }

    return true;
}

void Scaler::add_resampler(Resampler& resampler) {
    if (resamplers_.size() == resamplers_.max_size()) {
        roc_panic("scaler: attempting to add more than %u resamplers",
                  (unsigned)resamplers_.max_size());
    }

    resamplers_.append(&resampler);
}

packet::timestamp_t Scaler::queue_size_() const {
    if (!head_ || !tail_) {
        return 0;
    }

    packet::timestamp_t head_ts = head_->timestamp();
    packet::timestamp_t tail_ts = tail_->timestamp() //
        + (packet::timestamp_t)tail_->num_samples();

    packet::signed_timestamp_t dist = TS_SUBTRACT(tail_ts, head_ts);

    roc_panic_if(dist < 0);

    return (packet::timestamp_t)dist;
}

void Scaler::update_packet_(packet::IAudioPacketConstPtr& prev,
                            const packet::IPacketConstPtr& next) {
    if (!next) {
        return;
    }

    if (next->type() != packet::IAudioPacket::Type) {
        roc_panic("scaler: got packet of wrong type from (expected audio packet)");
    }

    const packet::IAudioPacket* next_ap =
        static_cast<const packet::IAudioPacket*>(next.get());

    if (prev && TS_IS_BEFORE(next_ap->timestamp(), prev->timestamp())) {
        return;
    }

    if (head_ && TS_IS_BEFORE(next_ap->timestamp(), head_->timestamp())) {
        return;
    }

    prev = next_ap;
}

} // namespace audio
} // namespace roc
