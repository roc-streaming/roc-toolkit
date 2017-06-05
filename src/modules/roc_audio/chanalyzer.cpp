/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/chanalyzer.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

Chanalyzer::Chanalyzer(packet::IPacketReader& packet_reader,
                       packet::channel_mask_t channel_mask)
    : packet_reader_(packet_reader)
    , channel_mask_(channel_mask)
    , head_(MaxChannels)
    , shift_pos_(MaxChannels)
    , shift_mask_(0)
    , min_shift_pos_(1) {
    if (channel_mask_ == 0) {
        roc_panic("chanalyzer: can't construct with zero channel mask");
    }

    for (packet::channel_t ch = 0; ch < MaxChannels; ch++) {
        new (readers_.allocate()) Reader(this, ch);
    }
}

packet::IPacketReader& Chanalyzer::reader(packet::channel_t ch) {
    if ((channel_mask_ & (1 << ch)) == 0) {
        roc_panic("chanalyzer: can't get reader for channel not in channel mask "
                  "(channel = %u, channel_mask = 0x%x)",
                  (unsigned)ch, (unsigned)channel_mask_);
    }

    return readers_[ch];
}

packet::IPacketConstPtr Chanalyzer::Reader::read() {
    roc_panic_if(!chanalyzer_);
    return chanalyzer_->read_(ch_);
}

packet::IPacketConstPtr Chanalyzer::read_(packet::channel_t ch) {
    roc_panic_if((channel_mask_ & (1 << ch)) == 0);

    if (packet_list_.size() == 0 || head_[ch] == packet_list_.back()) {
        if (!append_()) {
            return NULL;
        }
    }

    if (head_[ch]) {
        head_[ch] = packet_list_.next(*head_[ch]);
    } else {
        head_[ch] = packet_list_.front();
    }

    roc_panic_if(!head_[ch]);

    if (shift_pos_[ch]++ == min_shift_pos_) {
        shift_mask_ |= (1 << ch);

        if (shift_mask_ == channel_mask_) {
            shift_();
        }
    }

    return head_[ch];
}

bool Chanalyzer::append_() {
    packet::IPacketConstPtr packet = packet_reader_.read();

    if (!packet) {
        return false;
    }

    packet_list_.append(*packet);
    return true;
}

void Chanalyzer::shift_() {
    roc_panic_if(packet_list_.size() < 2);

    packet_list_.remove(*packet_list_.front());

    min_shift_pos_++;
    shift_mask_ = 0;

    for (size_t ch = 0; ch < MaxChannels; ch++) {
        if (ROC_IS_BEFORE(ssize_t, min_shift_pos_, shift_pos_[ch])) {
            shift_mask_ |= (1 << ch);
        }
    }
}

} // namespace audio
} // namespace roc
