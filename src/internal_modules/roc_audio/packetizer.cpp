/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/packetizer.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/secure_random.h"

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
    , samples_per_packet_(
          (packet::timestamp_t)sample_spec.timestamp_from_ns(packet_length))
    , payload_type_(payload_type)
    , payload_size_(payload_encoder.encoded_size(samples_per_packet_))
    , packet_pos_(0)
    , valid_(false) {
    uint32_t rand_source = 0, rand_seqnum = 0, rand_timestamp = 0;
    if (!core::secure_random(0, packet::source_t(-1), rand_source)
        || !core::secure_random(0, packet::seqnum_t(-1), rand_seqnum)
        || !core::secure_random(0, packet::timestamp_t(-1), rand_timestamp)) {
        roc_log(LogError, "packetizer: random generator failed");
        return;
    }
    source_ = (packet::source_t)rand_source;
    seqnum_ = (packet::seqnum_t)rand_seqnum;
    timestamp_ = (packet::timestamp_t)rand_timestamp;
    valid_ = true;
    roc_log(LogDebug, "packetizer: initializing: n_channels=%lu samples_per_packet=%lu",
            (unsigned long)sample_spec_.num_channels(),
            (unsigned long)samples_per_packet_);
}

bool Packetizer::valid() const {
    return valid_;
}

void Packetizer::write(Frame& frame) {
    if (frame.size() % sample_spec_.num_channels() != 0) {
        roc_panic("packetizer: unexpected frame size");
    }

    const sample_t* buffer_ptr = frame.data();
    size_t buffer_samples = frame.size() / sample_spec_.num_channels();

    while (buffer_samples != 0) {
        if (!packet_) {
            if (!begin_packet_()) {
                return;
            }
        }

        size_t ns = buffer_samples;
        if (ns > (samples_per_packet_ - packet_pos_)) {
            ns = (samples_per_packet_ - packet_pos_);
        }

        const size_t actual_ns =
            payload_encoder_.write(buffer_ptr, ns, sample_spec_.channel_mask());
        roc_panic_if_not(actual_ns == ns);

        buffer_ptr += actual_ns * sample_spec_.num_channels();
        buffer_samples -= actual_ns;

        packet_pos_ += actual_ns;

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
    rtp->timestamp = timestamp_;
    rtp->payload_type = payload_type_;

    packet_ = pp;

    return true;
}

void Packetizer::end_packet_() {
    payload_encoder_.end();

    packet_->rtp()->duration = (packet::timestamp_t)packet_pos_;

    if (packet_pos_ < samples_per_packet_) {
        pad_packet_();
    }

    writer_.write(packet_);

    seqnum_++;
    timestamp_ += (packet::timestamp_t)packet_pos_;

    packet_ = NULL;
    packet_pos_ = 0;
}

void Packetizer::pad_packet_() {
    const size_t actual_payload_size = payload_encoder_.encoded_size(packet_pos_);
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
