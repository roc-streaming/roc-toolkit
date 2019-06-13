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
#include "roc_rtp/format.h"
#include "roc_rtp/pcm_funcs.h"

namespace roc {
namespace rtp {

//! PCM encoder.
class PCMEncoder : public audio::IEncoder, public core::NonCopyable<> {
public:
    //! Initialize.
    PCMEncoder(const PCMFuncs& funcs, const Format& format);

    //! Calculate full packet size for given duration in nanoseconds.
    virtual size_t packet_size(core::nanoseconds_t duration) const;

    //! Calculate packet payload size for given number of samples.
    virtual size_t payload_size(size_t num_samples) const;

    //! Start encoding a new packet.
    virtual void begin(const packet::PacketPtr& packet);

    //! Encode samples.
    virtual size_t write(const audio::sample_t* samples,
                         size_t n_samples,
                         packet::channel_mask_t channels);

    //! Finish encoding packet.
    virtual void end();

private:
    const PCMFuncs& funcs_;

    packet::PacketPtr packet_;
    size_t packet_pos_;

    const size_t sample_rate_;

    const packet::source_t source_;
    const unsigned int payload_type_;

    packet::seqnum_t seqnum_;
    packet::timestamp_t timestamp_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PCM_ENCODER_H_
