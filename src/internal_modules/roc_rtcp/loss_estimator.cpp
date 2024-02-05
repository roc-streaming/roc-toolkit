/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/loss_estimator.h"

namespace roc {
namespace rtcp {

LossEstimator::LossEstimator()
    : prev_total_(0)
    , prev_lost_(0) {
}

float LossEstimator::update(const uint64_t total_packets, const int64_t lost_packets) {
    float fract_loss = 0;

    if (total_packets > prev_total_) {
        fract_loss =
            float(lost_packets - prev_lost_) / float(total_packets - prev_total_);
    }

    if (fract_loss < 0) {
        fract_loss = 0;
    }

    prev_total_ = total_packets;
    prev_lost_ = lost_packets;

    return fract_loss;
}

} // namespace rtcp
} // namespace roc
