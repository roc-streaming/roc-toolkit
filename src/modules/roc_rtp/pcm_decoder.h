/*
 * Copyright (c) 2015 Roc authors
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
#include "roc_core/panic.h"
#include "roc_rtp/format.h"
#include "roc_rtp/pcm_funcs.h"

namespace roc {
namespace rtp {

//! PCM decoder.
class PCMDecoder : public audio::IDecoder, public core::NonCopyable<> {
public:
    //! Initialize.
    PCMDecoder(const PCMFuncs& funcs, const Format& format);

    //! Start decoding a new packet.
    virtual void set(const packet::PacketPtr& packet);

    //! Get current stream position.
    virtual packet::timestamp_t timestamp() const;

    //! Get number of samples remaining in the current packet.
    virtual packet::timestamp_t remaining() const;

    //! Decode samples.
    virtual size_t
    read(audio::sample_t* samples, size_t n_samples, packet::channel_mask_t channels);

    //! Advance the stream position.
    virtual void advance(size_t n_samples);

private:
    const PCMFuncs& funcs_;

    packet::timestamp_t stream_pos_;

    packet::timestamp_t packet_pos_;
    packet::timestamp_t packet_rem_;

    packet::PacketPtr packet_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PCM_DECODER_H_
