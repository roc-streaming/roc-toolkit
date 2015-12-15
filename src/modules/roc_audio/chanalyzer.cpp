/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/helpers.h"
#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_audio/chanalyzer.h"

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
}

packet::IAudioPacketConstPtr Chanalyzer::read(packet::channel_t ch) {
    if ((channel_mask_ & (1 << ch)) == 0) {
        roc_panic("chanalyzer: can't read channel not in channel mask "
                  "(channel = %u, channel_mask = 0x%x)",
                  (unsigned)ch, (unsigned)channel_mask_);
    }

    if (packets_.size() == 0 || head_[ch] == packets_.back()) {
        if (!append_()) {
            return NULL;
        }
    }

    if (head_[ch]) {
        head_[ch] = packets_.next(*head_[ch]);
    } else {
        head_[ch] = packets_.front();
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
    packet::IPacketConstPtr packet;

    for (;;) {
        if (!(packet = packet_reader_.read())) {
            return false;
        }

        if (packet->type() == packet::IAudioPacket::Type) {
            break;
        }

        roc_log(LOG_TRACE, "chanalyzer: skipping non-audio packet from reader");
    }

    packet::IAudioPacketConstPtr audio =
        static_cast<const packet::IAudioPacket*>(packet.get());

    packets_.append(*audio);
    return true;
}

void Chanalyzer::shift_() {
    roc_panic_if(packets_.size() < 2);

    packets_.remove(*packets_.front());

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
