/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/packetizer.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace audio {

Packetizer::Packetizer(packet::IWriter& writer,
                       packet::IComposer& composer,
                       packet::ISequencer& sequencer,
                       IFrameEncoder& payload_encoder,
                       packet::PacketFactory& packet_factory,
                       core::nanoseconds_t packet_length,
                       const audio::SampleSpec& sample_spec)
    : writer_(writer)
    , composer_(composer)
    , sequencer_(sequencer)
    , payload_encoder_(payload_encoder)
    , packet_factory_(packet_factory)
    , sample_spec_(sample_spec)
    , samples_per_packet_(0)
    , payload_size_(0)
    , packet_pos_(0)
    , packet_cts_(0)
    , capture_ts_(0)
    , valid_(false) {
    roc_panic_if_msg(!sample_spec_.is_valid() || !sample_spec_.is_raw(),
                     "packetizer: required valid sample spec with raw format: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    if (packet_length <= 0 || sample_spec.ns_2_stream_timestamp(packet_length) <= 0) {
        roc_log(LogError,
                "packetizer: invalid config: packet length is invalid:"
                " packet_length=%.3fms samples_per_packet=%lu",
                (double)packet_length / core::Millisecond,
                (unsigned long)samples_per_packet_);
        return;
    }

    samples_per_packet_ = sample_spec.ns_2_stream_timestamp(packet_length);
    payload_size_ = payload_encoder.encoded_byte_count(samples_per_packet_);

    roc_log(
        LogDebug,
        "packetizer: initializing:"
        " packet_length=%.3fms samples_per_packet=%lu payload_size=%lu sample_spec=%s",
        (double)packet_length / core::Millisecond, (unsigned long)samples_per_packet_,
        (unsigned long)payload_size_, sample_spec_to_str(sample_spec_).c_str());

    valid_ = true;
}

bool Packetizer::is_valid() const {
    return valid_;
}

size_t Packetizer::sample_rate() const {
    return sample_spec_.sample_rate();
}

const PacketizerMetrics& Packetizer::metrics() const {
    return metrics_;
}

void Packetizer::write(Frame& frame) {
    if (frame.num_raw_samples() % sample_spec_.num_channels() != 0) {
        roc_panic("packetizer: unexpected frame size");
    }

    const sample_t* buffer_ptr = frame.raw_samples();
    size_t buffer_samples = frame.num_raw_samples() / sample_spec_.num_channels();
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

    packet_ = pp;
    packet_pos_ = 0;
    packet_cts_ = capture_ts_;

    // Begin encoding samples into packet.
    payload_encoder_.begin(packet_->payload().data(), packet_->payload().size());

    return true;
}

void Packetizer::end_packet_() {
    // How much bytes we've written into packet payload.
    const size_t written_payload_size = payload_encoder_.encoded_byte_count(packet_pos_);
    roc_panic_if_not(written_payload_size <= payload_size_);

    // Finish encoding samples into packet.
    payload_encoder_.end();

    // Fill protocol-specific fields.
    sequencer_.next(*packet_, packet_cts_, (packet::stream_timestamp_t)packet_pos_);

    // Apply padding if needed.
    if (packet_pos_ < samples_per_packet_) {
        pad_packet_(written_payload_size);
    }

    const status::StatusCode code = writer_.write(packet_);
    // TODO(gh-183): forward status
    roc_panic_if(code != status::StatusOK);

    metrics_.packet_count++;
    metrics_.payload_count += written_payload_size;

    packet_ = NULL;
    packet_pos_ = 0;
    packet_cts_ = 0;
}

void Packetizer::pad_packet_(size_t written_payload_size) {
    if (written_payload_size == payload_size_) {
        return;
    }

    if (!composer_.pad(*packet_, payload_size_ - written_payload_size)) {
        roc_panic("packetizer: can't pad packet: orig_size=%lu actual_size=%lu",
                  (unsigned long)payload_size_, (unsigned long)written_payload_size);
    }
}

packet::PacketPtr Packetizer::create_packet_() {
    packet::PacketPtr packet = packet_factory_.new_packet();
    if (!packet) {
        roc_log(LogError, "packetizer: can't allocate packet");
        return NULL;
    }

    packet->add_flags(packet::Packet::FlagAudio);

    core::Slice<uint8_t> buffer = packet_factory_.new_packet_buffer();
    if (!buffer) {
        roc_log(LogError, "packetizer: can't allocate buffer");
        return NULL;
    }

    if (!composer_.prepare(*packet, buffer, payload_size_)) {
        roc_log(LogError, "packetizer: can't prepare packet");
        return NULL;
    }
    packet->add_flags(packet::Packet::FlagPrepared);

    packet->set_buffer(buffer);

    return packet;
}

} // namespace audio
} // namespace roc
