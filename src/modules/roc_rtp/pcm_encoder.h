/*
 * Copyright (c) 2015 Roc authors
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
#include "roc_rtp/pcm_funcs.h"

namespace roc {
namespace rtp {

//! PCM encoder.
class PCMEncoder : public audio::IEncoder, public core::NonCopyable<> {
public:
    //! Initialize.
    PCMEncoder(const PCMFuncs& funcs);

    //! Get packet payload size.
    virtual size_t payload_size(size_t num_samples) const;

    //! Write samples to packet.
    virtual size_t write_samples(packet::Packet& packet,
                                 size_t offset,
                                 const audio::sample_t* samples,
                                 size_t n_samples,
                                 packet::channel_mask_t channels);

private:
    const PCMFuncs& funcs_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PCM_ENCODER_H_
