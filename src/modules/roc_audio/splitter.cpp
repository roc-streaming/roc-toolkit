/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/math.h"
#include "roc_core/panic.h"
#include "roc_core/random.h"

#include "roc_audio/splitter.h"

namespace roc {
namespace audio {

Splitter::Splitter(packet::IPacketWriter& output,
                   packet::IPacketComposer& composer,
                   size_t samples,
                   packet::channel_mask_t channels,
                   size_t rate)
    : output_(output)
    , composer_(composer)
    , channels_(channels)
    , n_channels_(packet::num_channels(channels))
    , n_packet_samples_(samples)
    , rate_(rate)
    , source_((packet::source_t)core::random(packet::source_t(-1)))
    , seqnum_((packet::seqnum_t)core::random(packet::seqnum_t(-1)))
    , timestamp_((packet::timestamp_t)core::random(packet::timestamp_t(-1)))
    , n_samples_(0) {
}

void Splitter::write(const ISampleBufferConstSlice& buffer) {
    if (!buffer) {
        roc_panic("attempting to write empty buffer to splitter");
    }

    const packet::sample_t* buffer_pos = buffer.data();

    const size_t buffer_size = buffer.size();

    if (buffer_size % n_channels_ != 0) {
        roc_panic(
            "attempting to write buffer which size is not multiple of # of channels "
            "(buff_sz = %lu, n_ch = %lu)",
            (unsigned long)buffer_size, (unsigned long)n_channels_);
    }

    size_t samples_to_write = buffer_size / n_channels_;

    while (samples_to_write != 0) {
        if (!packet_ && !create_packet_()) {
            return;
        }

        roc_panic_if_not(n_samples_ < n_packet_samples_);

        const size_t ns = ROC_MIN(samples_to_write, n_packet_samples_ - n_samples_);

        packet_->audio()->write_samples(channels_, n_samples_, buffer_pos, ns);

        n_samples_ += ns;
        samples_to_write -= ns;

        buffer_pos += (ns * n_channels_);

        if (n_samples_ == n_packet_samples_) {
            flush();
        }
    }
}

void Splitter::flush() {
    if (packet_) {
        output_.write(packet_);
        packet_ = NULL;
        n_samples_ = 0;
        seqnum_++;
        timestamp_ += n_packet_samples_;
    }
}

bool Splitter::create_packet_() {
    roc_panic_if_not(n_samples_ == 0);

    packet::IPacketPtr pp =
        composer_.compose(packet::IPacket::HasRTP | packet::IPacket::HasAudio);

    if (!pp) {
        roc_log(LogError, "splitter: composer returned null");
        return false;
    }

    if (!pp->rtp()) {
        roc_panic("splitter: composer returned packet w/o RTP header");
    }

    if (!pp->audio()) {
        roc_panic("splitter: composer returned packet w/o audio payload");
    }

    packet_ = pp;

    packet_->rtp()->set_source(source_);
    packet_->rtp()->set_seqnum(seqnum_);
    packet_->rtp()->set_timestamp(timestamp_);
    packet_->audio()->configure(channels_, n_packet_samples_, rate_);

    return true;
}

} // namespace audio
} // namespace roc
