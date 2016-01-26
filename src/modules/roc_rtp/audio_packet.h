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
#include "roc_rtp/rtp_packet.h"
#include "roc_rtp/audio_format.h"

namespace roc {
namespace rtp {

//! RTP audio packet.
class AudioPacket : public core::NonCopyable<>,
                    public packet::IPacket,
                    private packet::IHeaderOrdering,
                    private packet::IHeaderRTP,
                    private packet::IPayloadAudio {
public:
    //! Initialize.
    AudioPacket(core::IPool<AudioPacket>&, const RTP_Packet&, const AudioFormat*);

    //! Get packet options.
    virtual int options() const;

    //! Get ordering header.
    virtual const packet::IHeaderOrdering* order() const;

    //! Get RTP header.
    virtual const packet::IHeaderRTP* rtp() const;

    //! Get RTP header.
    virtual packet::IHeaderRTP* rtp();

    //! Get FECFRAME header.
    virtual const packet::IHeaderFECFrame* fec() const;

    //! Get FECFRAME header.
    virtual packet::IHeaderFECFrame* fec();

    //! Get audio payload.
    virtual const packet::IPayloadAudio* audio() const;

    //! Get audio payload.
    virtual packet::IPayloadAudio* audio();

    //! Get packet data buffer (containing header and payload).
    virtual core::IByteBufferConstSlice raw_data() const;

    //! Get payload.
    virtual core::IByteBufferConstSlice payload() const;

    //! Set payload data and size.
    virtual void set_payload(const uint8_t* data, size_t size);

private:
    virtual void free();

    virtual bool is_same_flow(const packet::IPacket& other) const;
    virtual bool is_before(const packet::IPacket& other) const;

    virtual packet::source_t source() const;
    virtual void set_source(packet::source_t);

    virtual packet::seqnum_t seqnum() const;
    virtual void set_seqnum(packet::seqnum_t);

    virtual packet::timestamp_t timestamp() const;
    virtual void set_timestamp(packet::timestamp_t);

    virtual size_t rate() const;

    virtual bool marker() const;
    virtual void set_marker(bool);

    virtual packet::channel_mask_t channels() const;
    virtual size_t num_samples() const;

    virtual void configure(packet::channel_mask_t ch_mask, size_t n_samples, size_t rate);

    virtual size_t read_samples(packet::channel_mask_t ch_mask,
                                size_t offset,
                                packet::sample_t* samples,
                                size_t n_samples) const;

    virtual void write_samples(packet::channel_mask_t ch_mask,
                               size_t offset,
                               const packet::sample_t* samples,
                               size_t n_samples);

    RTP_Packet packet_;
    const AudioFormat* format_;
    core::IPool<AudioPacket>& pool_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_AUDIO_PACKET_H_
