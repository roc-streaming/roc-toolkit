/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/pcm_encoder.h"
#include "roc_core/panic.h"
#include "roc_core/random.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

PCMEncoder::PCMEncoder(const PCMFuncs& funcs, const Format& format)
    : funcs_(funcs)
    , packet_pos_(0)
    , sample_rate_(format.sample_rate)
    , source_((packet::source_t)core::random(packet::source_t(-1)))
    , payload_type_(format.payload_type)
    , seqnum_((packet::seqnum_t)core::random(packet::seqnum_t(-1)))
    , timestamp_((packet::timestamp_t)core::random(packet::timestamp_t(-1))) {
}

size_t PCMEncoder::packet_size(core::nanoseconds_t duration) const {
    const packet::timestamp_diff_t num_samples =
        packet::timestamp_from_ns(duration, sample_rate_);
    if (num_samples < 0) {
        return 0;
    }
    return sizeof(Header) + funcs_.payload_size_from_samples((size_t)num_samples);
}

size_t PCMEncoder::payload_size(size_t num_samples) const {
    return funcs_.payload_size_from_samples(num_samples);
}

void PCMEncoder::begin(const packet::PacketPtr& packet) {
    if (packet_) {
        roc_panic("pcm encoder: unpaired begin/end");
    }

    packet::RTP* rtp = packet->rtp();
    if (!rtp) {
        roc_panic("pcm encoder: unexpected non-rtp packet");
    }

    rtp->source = source_;
    rtp->seqnum = seqnum_;
    rtp->timestamp = timestamp_;
    rtp->payload_type = payload_type_;

    packet_ = packet;
}

size_t PCMEncoder::write(const audio::sample_t* samples,
                         size_t n_samples,
                         packet::channel_mask_t channels) {
    if (!packet_) {
        roc_panic("pcm encoder: write() should be called only between begin() and end()");
    }

    packet::RTP& rtp = *packet_->rtp();

    const size_t wr_samples =
        funcs_.encode_samples(rtp.payload.data(), rtp.payload.size(), packet_pos_,
                              samples, n_samples, channels);

    packet_pos_ += wr_samples;
    return wr_samples;
}

void PCMEncoder::end() {
    if (!packet_) {
        roc_panic("pcm encoder: unpaired begin/end");
    }

    packet::RTP& rtp = *packet_->rtp();

    rtp.duration = (packet::timestamp_t)packet_pos_;

    // TODO: zeroize and setup padding if necessary

    seqnum_++;
    timestamp_ += (packet::timestamp_t)packet_pos_;

    packet_pos_ = 0;
    packet_ = NULL;
}

} // namespace rtp
} // namespace roc
