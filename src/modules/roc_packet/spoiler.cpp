/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/random.h"
#include "roc_core/time.h"

#include "roc_packet/spoiler.h"

namespace roc {
namespace packet {

Spoiler::Spoiler(IPacketWriter& writer)
    : writer_(writer)
    , loss_rate_(0)
    , delay_rate_(0)
    , delay_ms_(0) {
}

void Spoiler::set_random_loss(size_t rate) {
    if (rate > 100) {
        roc_panic("random loss rate should be in range [0; 100]");
    }
    loss_rate_ = rate;
}

void Spoiler::set_random_delay(size_t rate, size_t ms) {
    if (rate > 100) {
        roc_panic("random delay rate should be in range [0; 100]");
    }
    delay_rate_ = rate;
    delay_ms_ = ms;
}

void Spoiler::write(const IPacketPtr& packet) {
    // FIXME: remove this when we finish FECFRAME support
    if (!packet->rtp() || !packet->rtp()->marker()) {
        if (core::random(100) < loss_rate_) {
            return;
        }
    }
    if (core::random(100) < delay_rate_) {
        core::sleep_for_ms(delay_ms_);
    }
    writer_.write(packet);
}

} // namespace packet
} // namespace roc
