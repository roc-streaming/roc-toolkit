/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/packetizer.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/random.h"

namespace roc {
namespace audio {

Packetizer::Packetizer(packet::IWriter& writer,
                       packet::IComposer& composer,
                       IEncoder& encoder,
                       packet::PacketPool& packet_pool,
                       core::BufferPool<uint8_t>& buffer_pool,
                       packet::channel_mask_t channels,
                       core::nanoseconds_t packet_length,
                       size_t sample_rate)
    : writer_(writer)
    , composer_(composer)
    , encoder_(encoder)
    , packet_pool_(packet_pool)
    , buffer_pool_(buffer_pool)
    , channels_(channels)
    , num_channels_(packet::num_channels(channels))
    , samples_per_packet_(
          (packet::timestamp_t)packet::timestamp_from_ns(packet_length, sample_rate))
    , remaining_samples_(0) {
    roc_log(LogDebug, "packetizer: initializing: n_channels=%lu samples_per_packet=%lu",
            (unsigned long)num_channels_, (unsigned long)samples_per_packet_);
}

void Packetizer::write(Frame& frame) {
    if (frame.size() % num_channels_ != 0) {
        roc_panic("packetizer: unexpected frame size");
    }

    if (frame.size() == 0) {
        return;
    }

    const sample_t* buffer_ptr = frame.data();
    size_t buffer_samples = frame.size() / num_channels_;

    while (buffer_samples != 0) {
        if (!packet_) {
            if (!begin_packet_()) {
                return;
            }
        }

        size_t ns = buffer_samples;
        if (ns > remaining_samples_) {
            ns = remaining_samples_;
        }

        const size_t actual_ns = encoder_.write(buffer_ptr, ns, channels_);
        roc_panic_if_not(actual_ns <= ns);

        buffer_ptr += actual_ns * num_channels_;
        buffer_samples -= actual_ns;

        remaining_samples_ -= actual_ns;

        if (actual_ns < ns || remaining_samples_ == 0) {
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

    encoder_.begin(pp);

    packet_ = pp;
    remaining_samples_ = samples_per_packet_;

    return true;
}

void Packetizer::end_packet_() {
    encoder_.end();

    writer_.write(packet_);
    packet_ = NULL;
}

packet::PacketPtr Packetizer::create_packet_() {
    packet::PacketPtr packet = new (packet_pool_) packet::Packet(packet_pool_);
    if (!packet) {
        roc_log(LogError, "packetizer: can't allocate packet");
        return NULL;
    }

    packet->add_flags(packet::Packet::FlagAudio);

    core::Slice<uint8_t> data = new (buffer_pool_) core::Buffer<uint8_t>(buffer_pool_);
    if (!data) {
        roc_log(LogError, "packetizer: can't allocate buffer");
        return NULL;
    }

    if (!composer_.prepare(*packet, data, encoder_.payload_size(samples_per_packet_))) {
        roc_log(LogError, "packetizer: can't prepare packet");
        return NULL;
    }

    packet->set_data(data);

    return packet;
}

} // namespace audio
} // namespace roc
