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
#include "roc_packet/iaudio_packet.h"
#include "roc_rtp/rtp_packet.h"
#include "roc_rtp/rtp_audio_format.h"

namespace roc {
namespace rtp {

//! RTP audio packet.
class AudioPacket : public packet::IAudioPacket, public core::NonCopyable<> {
public:
    //! Initialize.
    AudioPacket(core::IPool<AudioPacket>&, const RTP_Packet&, const RTP_AudioFormat*);

    //! Get packet source ID.
    virtual packet::source_t source() const;

    //! Set packet source ID.
    virtual void set_source(packet::source_t);

    //! Get packet sequence number.
    virtual packet::seqnum_t seqnum() const;

    //! Set packet sequence number.
    virtual void set_seqnum(packet::seqnum_t);

    //! Get packet timestamp.
    virtual packet::timestamp_t timestamp() const;

    //! Set packet timestamp.
    virtual void set_timestamp(packet::timestamp_t);

    //! Get packet marker bit.
    virtual bool marker() const;

    //! Set packet marker bit.
    virtual void set_marker(bool);

    //! Get bitmask of channels present in packet.
    virtual packet::channel_mask_t channels() const;

    //! Get number of samples in packet.
    virtual size_t num_samples() const;

    //! Set channel mask and number of samples per channel.
    virtual void set_size(packet::channel_mask_t ch_mask, size_t n_samples);

    //! Read samples from packet.
    virtual size_t read_samples(packet::channel_mask_t ch_mask,
                                size_t offset,
                                packet::sample_t* samples,
                                size_t n_samples) const;

    //! Write samples to packet.
    virtual void write_samples(packet::channel_mask_t ch_mask,
                               size_t offset,
                               const packet::sample_t* samples,
                               size_t n_samples);

    //! Get packet data buffer (containing header and payload).
    virtual core::IByteBufferConstSlice raw_data() const;

private:
    virtual void free();

    RTP_Packet packet_;
    const RTP_AudioFormat* format_;
    core::IPool<AudioPacket>& pool_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_AUDIO_PACKET_H_
