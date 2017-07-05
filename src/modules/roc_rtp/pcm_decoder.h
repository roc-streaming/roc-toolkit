/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/pcm_decoder.h
//! @brief PCM decoder.

#ifndef ROC_RTP_PCM_DECODER_H_
#define ROC_RTP_PCM_DECODER_H_

#include "roc_audio/idecoder.h"
#include "roc_core/iallocator.h"
#include "roc_rtp/pcm_helpers.h"

namespace roc {
namespace rtp {

//! PCM decoder.
template <class Sample, size_t NumCh>
class PCMDecoder : public audio::IDecoder, public core::NonCopyable<> {
public:
    //! Create decoder.
    static audio::IDecoder* create(core::IAllocator& allocator) {
        return new (allocator) PCMDecoder;
    }

    //! Read samples from packet.
    virtual size_t read_samples(const packet::Packet& packet,
                                size_t offset,
                                audio::sample_t* samples,
                                size_t n_samples,
                                packet::channel_mask_t channels) {
        return pcm_read<Sample, NumCh>(packet.rtp()->payload.data(),
                                       packet.rtp()->payload.size(), offset, samples,
                                       n_samples, channels);
    }
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PCM_DECODER_H_
