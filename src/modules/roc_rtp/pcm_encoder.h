/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/pcm_encoder.h
//! @brief PCM encoder.

#ifndef ROC_RTP_PCM_ENCODER_H_
#define ROC_RTP_PCM_ENCODER_H_

#include "roc_audio/iencoder.h"
#include "roc_core/iallocator.h"
#include "roc_rtp/pcm_helpers.h"

namespace roc {
namespace rtp {

//! PCM encoder.
template <class Sample, size_t NumCh>
class PCMEncoder : public audio::IEncoder, public core::NonCopyable<> {
public:
    //! Create encoder.
    static audio::IEncoder* create(core::IAllocator& allocator) {
        return new (allocator) PCMEncoder;
    }

    //! Get packet payload size.
    virtual size_t payload_size(size_t num_samples) const {
        return pcm_payload_size<Sample, NumCh>(num_samples);
    }

    //! Write samples to packet.
    virtual size_t write_samples(packet::Packet& packet,
                                 size_t offset,
                                 const audio::sample_t* samples,
                                 size_t n_samples,
                                 packet::channel_mask_t channels) {
        return pcm_write<Sample, NumCh>(packet.rtp()->payload.data(),
                                        packet.rtp()->payload.size(), offset, samples,
                                        n_samples, channels);
    }
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PCM_ENCODER_H_
