/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/audio_packet.h
//! @brief RTP audio packet.

#ifndef ROC_RTP_AUDIO_PACKET_H_
#define ROC_RTP_AUDIO_PACKET_H_

#include "roc_core/noncopyable.h"
#include "roc_core/ipool.h"
#include "roc_packet/ipacket.h"
#include "roc_rtp/packet.h"
#include "roc_rtp/audio_format.h"

namespace roc {
namespace rtp {

//! RTP audio packet.
class AudioPacket : public Packet, private packet::IPayloadAudio {
public:
    //! Initialize.
    AudioPacket(core::IPool<AudioPacket>&, const AudioFormat*);

    //! Get packet options.
    virtual int options() const;

    //! Get audio payload.
    virtual const packet::IPayloadAudio* audio() const;

    //! Get audio payload.
    virtual packet::IPayloadAudio* audio();

private:
    virtual void free();

    virtual packet::channel_mask_t channels() const;
    virtual size_t num_samples() const;
    virtual size_t rate() const;

    virtual void configure(packet::channel_mask_t ch_mask, size_t n_samples, size_t rate);

    virtual size_t read_samples(packet::channel_mask_t ch_mask,
                                size_t offset,
                                packet::sample_t* samples,
                                size_t n_samples) const;

    virtual void write_samples(packet::channel_mask_t ch_mask,
                               size_t offset,
                               const packet::sample_t* samples,
                               size_t n_samples);

    const AudioFormat* format_;
    core::IPool<AudioPacket>& pool_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_AUDIO_PACKET_H_
