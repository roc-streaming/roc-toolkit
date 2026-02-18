/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/packet_counter.h"

namespace roc {
namespace rtcp {

PacketCounter::PacketCounter()
    : first_update_(true)
    , begin64_(0)
    , end64_hi_(0)
    , end64_lo_(0)
    , counter_(0) {
}

uint64_t PacketCounter::update(const uint32_t begin, const uint32_t end) {
    // If this is first update, or begin was changed, reset state.
    if (first_update_ || begin != begin64_) {
        begin64_ = begin;
        end64_hi_ = 0;
        end64_lo_ = end;
        first_update_ = false;
    }

    // Update end.
    if (int32_t(end - end64_lo_) > 0) {
        if (end < end64_lo_) {
            end64_hi_ += (uint64_t)1 << 32;
        }
        end64_lo_ = end;
    }

    // Update counter.
    if (begin64_ <= (end64_hi_ + end64_lo_)) {
        counter_ = (end64_hi_ + end64_lo_) - begin64_;
    }

    return counter_;
}

} // namespace rtcp
} // namespace roc
