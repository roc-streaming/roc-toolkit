/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/stddefs.h"
#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_core/math.h"

#include "roc_rtp/audio_packet.h"

namespace roc {
namespace rtp {

AudioPacket::AudioPacket(core::IPool<AudioPacket>& pool,
                         const RTP_Packet& packet,
                         const RTP_AudioFormat* format)
    : packet_(packet)
    , format_(format)
    , pool_(pool) {
}

void AudioPacket::free() {
    pool_.destroy(*this);
}

packet::source_t AudioPacket::source() const {
    return packet_.header().ssrc();
}

void AudioPacket::set_source(packet::source_t s) {
    packet_.header().set_ssrc(s);
}

packet::seqnum_t AudioPacket::seqnum() const {
    return packet_.header().seqnum();
}

void AudioPacket::set_seqnum(packet::seqnum_t sn) {
    packet_.header().set_seqnum(sn);
}

packet::timestamp_t AudioPacket::timestamp() const {
    return packet_.header().timestamp();
}

void AudioPacket::set_timestamp(packet::timestamp_t ts) {
    packet_.header().set_timestamp(ts);
}

bool AudioPacket::marker() const {
    return packet_.header().marker();
}

void AudioPacket::set_marker(bool m) {
    packet_.header().set_marker(m);
}

void AudioPacket::set_size(packet::channel_mask_t ch_mask, size_t n_samples) {
    if (const RTP_AudioFormat* format = get_audio_format_ch(ch_mask)) {
        format_ = format;
    } else {
        roc_panic("rtp audio packet: no supported format for channel mask 0x%xu",
                  (unsigned)ch_mask);
    }

    packet_.header().set_payload_type(format_->pt);
    packet_.set_payload_size(format_->size(n_samples));

    if (n_samples) {
        format_->clear(packet_.payload().data(), n_samples);
    }

    roc_panic_if(channels() != ch_mask);
    roc_panic_if(num_samples() != n_samples);
}

packet::channel_mask_t AudioPacket::channels() const {
    if (!format_) {
        roc_panic("rtp audio packet: audio format isn't set, forgot set_size()?");
    }
    return format_->channels;
}

size_t AudioPacket::num_samples() const {
    if (!format_) {
        roc_panic("rtp audio packet: audio format isn't set, forgot set_size()?");
    }
    return format_->n_samples(packet_.payload().size());
}

size_t AudioPacket::read_samples(packet::channel_mask_t ch_mask,
                                 size_t offset,
                                 packet::sample_t* samples,
                                 size_t n_samples) const {
    if (!format_) {
        roc_panic("rtp audio packet: audio format isn't set, forgot set_size()?");
    }

    if (!samples) {
        roc_panic("rtp audio packet: samples buffer is null");
    }

    const size_t max_samples = format_->n_samples(packet_.payload().size());

    offset = ROC_MIN(max_samples, offset);
    n_samples = ROC_MIN(max_samples - offset, n_samples);

    if (n_samples && ch_mask) {
        format_->read(packet_.payload().data(), offset, ch_mask, samples, n_samples);
    }

    return n_samples;
}

void AudioPacket::write_samples(packet::channel_mask_t ch_mask,
                                size_t offset,
                                const packet::sample_t* samples,
                                size_t n_samples) {
    if (!format_) {
        roc_panic("rtp audio packet: audio format isn't set, forgot set_size()?");
    }

    if (!samples) {
        roc_panic("rtp audio packet: samples buffer is null");
    }

    const size_t max_samples = format_->n_samples(packet_.payload().size());

    if (offset > max_samples) {
        roc_panic("rtp audio packet: offset out of bounds: got=%u max=%u",
                  (unsigned)offset, (unsigned)max_samples);
    }

    if (offset + n_samples > max_samples) {
        roc_panic("rtp audio packet: n_samples out of bounds: got=%u max=%u",
                  (unsigned)n_samples, (unsigned)(max_samples - offset));
    }

    if (n_samples && ch_mask) {
        format_->write(packet_.payload().data(), offset, ch_mask, samples, n_samples);
    }
}

core::IByteBufferConstSlice AudioPacket::raw_data() const {
    if (!format_) {
        roc_panic("rtp audio packet: audio format isn't set, forgot set_size()?");
    }
    return packet_.raw_data();
}

} // namespace rtp
} // namespace roc
