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
    : funcs_(funcs) {
}

size_t PCMDecoder::read_samples(const packet::Packet& packet,
                                size_t offset,
                                audio::sample_t* samples,
                                size_t n_samples,
                                packet::channel_mask_t channels) {
    const packet::RTP* rtp = packet.rtp();
    if (!rtp) {
        roc_panic("unexpected non-rtp packet");
    }

    return funcs_.decode_samples(rtp->payload.data(), rtp->payload.size(), offset,
                                 samples, n_samples, channels);
}

} // namespace rtp
} // namespace roc
