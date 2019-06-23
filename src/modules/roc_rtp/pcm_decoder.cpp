/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/pcm_decoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

PCMDecoder::PCMDecoder(const PCMFuncs& funcs, const Format&)
    : funcs_(funcs)
    , stream_pos_(0)
    , packet_pos_(0)
    , packet_rem_(0) {
}

void PCMDecoder::set(const packet::PacketPtr& packet) {
    roc_panic_if(!packet);

    packet::RTP* rtp = packet->rtp();
    if (!rtp) {
        roc_panic("pcm decoder: unexpected non-rtp packet");
    }

    stream_pos_ = rtp->timestamp;
    packet_pos_ = 0;
    packet_rem_ = rtp->duration;

    packet_ = packet;
}

packet::timestamp_t PCMDecoder::timestamp() const {
    if (!packet_) {
        roc_panic("pcm decoder: position() should be called after set()");
    }

    return stream_pos_;
}

packet::timestamp_t PCMDecoder::remaining() const {
    if (!packet_) {
        roc_panic("pcm decoder: remaining() should be called after set()");
    }

    return packet_rem_;
}

size_t PCMDecoder::read(audio::sample_t* samples,
                        size_t n_samples,
                        packet::channel_mask_t channels) {
    if (!packet_) {
        roc_panic("pcm decoder: read() should be called after set()");
    }

    if (n_samples > (size_t)packet_rem_) {
        n_samples = (size_t)packet_rem_;
    }

    packet::RTP& rtp = *packet_->rtp();

    const size_t rd_samples =
        funcs_.decode_samples(rtp.payload.data(), rtp.payload.size(), packet_pos_,
                              samples, n_samples, channels);

    advance(rd_samples);

    return rd_samples;
}

void PCMDecoder::advance(size_t n_samples) {
    packet::timestamp_t ns = (packet::timestamp_t)n_samples;

    stream_pos_ += ns;
    packet_pos_ += ns;

    if (ns > packet_rem_) {
        ns = packet_rem_;
    }

    packet_rem_ -= ns;
}

} // namespace rtp
} // namespace roc
