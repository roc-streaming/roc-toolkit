/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_encoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PCMEncoder::PCMEncoder(const PCMFuncs& funcs)
    : funcs_(funcs) {
}

size_t PCMEncoder::payload_size(size_t num_samples) const {
    return funcs_.payload_size_from_samples(num_samples);
}

size_t PCMEncoder::write_samples(packet::Packet& packet,
                                 size_t offset,
                                 const sample_t* samples,
                                 size_t n_samples,
                                 packet::channel_mask_t channels) {
    packet::RTP* rtp = packet.rtp();
    if (!rtp) {
        roc_panic("unexpected non-rtp packet");
    }
    return funcs_.encode_samples(rtp->payload.data(), rtp->payload.size(), offset,
                                 samples, n_samples, channels);
}

} // namespace audio
} // namespace roc
