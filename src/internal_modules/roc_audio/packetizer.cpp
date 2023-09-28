/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/packetizer.h"
#include "roc_core/fast_random.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

Packetizer::Packetizer(packet::IWriter& writer,
                       packet::IComposer& composer,
                       IFrameEncoder& payload_encoder,
                       packet::PacketFactory& packet_factory,
                       core::BufferFactory<uint8_t>& buffer_factory,
                       core::nanoseconds_t packet_length,
                       const audio::SampleSpec& sample_spec,
                       unsigned int payload_type)
    : writer_(writer)
    , composer_(composer)
    , payload_encoder_(payload_encoder)
    , packet_factory_(packet_factory)
    , buffer_factory_(buffer_factory)
    , sample_spec_(sample_spec)
    , samples_per_packet_((packet::stream_timestamp_t)
                              sample_spec.ns_2_stream_timestamp_delta(packet_length))
    , payload_type_(payload_type)
    , payload_size_(payload_encoder.encoded_byte_count(samples_per_packet_))
    , packet_pos_(0)
    , valid_(false) {
    source_ = (packet::stream_source_t)core::fast_random(0, packet::stream_source_t(-1));
    seqnum_ = (packet::seqnum_t)core::fast_random(0, packet::seqnum_t(-1));
    stream_ts_ =
        (packet::stream_timestamp_t)core::fast_random(0, packet::stream_timestamp_t(-1));
    capture_ts_ = 0;
    valid_ = true;
    roc_log(LogDebug, "packetizer: initializing: n_channels=%lu samples_per_packet=%lu",
            (unsigned long)sample_spec_.num_channels(),
            (unsigned long)samples_per_packet_);
}

bool Packetizer::is_valid() const {
    return valid_;
}

void Packetizer::write(Frame& frame) {
    if (frame.num_samples() % sample_spec_.num_channels() != 0) {
        roc_panic("packetizer: unexpected frame size");
    }

    const sample_t* buffer_ptr = frame.samples();
    size_t buffer_samples = frame.num_samples() / sample_spec_.num_channels();
    capture_ts_ = frame.capture_timestamp();

    while (buffer_samples != 0) {
        if (!packet_) {
            if (!begin_packet_()) {
                return;
            }
        }

        const size_t n_requested =
            std::min(buffer_samples, samples_per_packet_ - packet_pos_);

        const size_t n_encoded = payload_encoder_.write(buffer_ptr, n_requested);
        roc_panic_if_not(n_encoded == n_requested);

        buffer_ptr += n_encoded * sample_spec_.num_channels();
        buffer_samples -= n_encoded;

        packet_pos_ += n_encoded;
        if (capture_ts_) {
            capture_ts_ += sample_spec_.samples_per_chan_2_ns(n_encoded);
        }

        if (packet_pos_ == samples_per_packet_) {
            end_packet_();
        }
    }
}

void Packetizer::flush() {
    if (packet_) {
        end_packet_();
    }
}

bool Packetizer::begin_packet_() {
    packet::PacketPtr pp = create_packet_();
    if (!pp) {
        return false;
    }

    packet::RTP* rtp = pp->rtp();
    if (!rtp) {
        roc_panic("packetizer: unexpected non-rtp packet");
    }

    payload_encoder_.begin(rtp->payload.data(), rtp->payload.size());

    rtp->source = source_;
    rtp->seqnum = seqnum_;
    rtp->stream_timestamp = stream_ts_;
    rtp->capture_timestamp = capture_ts_;
    rtp->payload_type = payload_type_;

    packet_ = pp;

    return true;
}

void Packetizer::end_packet_() {
    payload_encoder_.end();

    packet_->rtp()->duration = (packet::stream_timestamp_t)packet_pos_;

    if (packet_pos_ < samples_per_packet_) {
        pad_packet_();
    }

    writer_.write(packet_);

    seqnum_++;
    stream_ts_ += (packet::stream_timestamp_t)packet_pos_;

    packet_ = NULL;
    packet_pos_ = 0;
}

void Packetizer::pad_packet_() {
    const size_t actual_payload_size = payload_encoder_.encoded_byte_count(packet_pos_);
    roc_panic_if_not(actual_payload_size <= payload_size_);

    if (actual_payload_size == payload_size_) {
        return;
    }

    if (!composer_.pad(*packet_, payload_size_ - actual_payload_size)) {
        roc_panic("packetizer: can't pad packet: orig_size=%lu actual_size=%lu",
                  (unsigned long)payload_size_, (unsigned long)actual_payload_size);
    }
}

packet::PacketPtr Packetizer::create_packet_() {
    packet::PacketPtr packet = packet_factory_.new_packet();
    if (!packet) {
        roc_log(LogError, "packetizer: can't allocate packet");
        return NULL;
    }

    packet->add_flags(packet::Packet::FlagAudio);

    core::Slice<uint8_t> data = buffer_factory_.new_buffer();
    if (!data) {
        roc_log(LogError, "packetizer: can't allocate buffer");
        return NULL;
    }

    if (!composer_.prepare(*packet, data, payload_size_)) {
        roc_log(LogError, "packetizer: can't prepare packet");
        return NULL;
    }

    packet->set_data(data);

    return packet;
}

} // namespace audio
} // namespace roc
