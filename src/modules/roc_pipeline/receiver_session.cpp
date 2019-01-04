/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_session.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#ifdef ROC_TARGET_OPENFEC
#include "roc_fec/of_decoder.h"
#endif

namespace roc {
namespace pipeline {

ReceiverSession::ReceiverSession(const SessionConfig& config,
                                 const unsigned int payload_type,
                                 const size_t out_sample_rate,
                                 const packet::Address& src_address,
                                 const rtp::FormatMap& format_map,
                                 packet::PacketPool& packet_pool,
                                 core::BufferPool<uint8_t>& byte_buffer_pool,
                                 core::BufferPool<audio::sample_t>& sample_buffer_pool,
                                 core::IAllocator& allocator)
    : src_address_(src_address)
    , allocator_(allocator)
    , audio_reader_(NULL) {
    const rtp::Format* format = format_map.format(payload_type);
    if (!format) {
        return;
    }

    queue_router_.reset(new (allocator_) packet::Router(allocator_, 2), allocator_);
    if (!queue_router_ || !queue_router_->valid()) {
        return;
    }

    source_queue_.reset(new (allocator_) packet::SortedQueue(0), allocator_);
    if (!source_queue_) {
        return;
    }

    packet::IWriter* pwriter = source_queue_.get();

    if (!queue_router_->add_route(*pwriter, packet::Packet::FlagAudio)) {
        return;
    }

    packet::IReader* preader = source_queue_.get();

    delayed_reader_.reset(
        new (allocator_) packet::DelayedReader(*preader, config.latency), allocator_);
    if (!delayed_reader_) {
        return;
    }
    preader = delayed_reader_.get();

    validator_.reset(new (allocator_)
                         rtp::Validator(*preader, *format, config.rtp_validator),
                     allocator_);
    if (!validator_) {
        return;
    }
    preader = validator_.get();

#ifdef ROC_TARGET_OPENFEC
    if (config.fec.codec != fec::NoCodec) {
        repair_queue_.reset(new (allocator_) packet::SortedQueue(0), allocator_);
        if (!repair_queue_) {
            return;
        }
        if (!queue_router_->add_route(*repair_queue_, packet::Packet::FlagRepair)) {
            return;
        }

        core::UniquePtr<fec::OFDecoder> fec_decoder(
            new (allocator_)
                fec::OFDecoder(config.fec, format->size(config.samples_per_packet),
                               byte_buffer_pool, allocator_),
            allocator_);
        if (!fec_decoder || !fec_decoder->valid()) {
            return;
        }
        fec_decoder_.reset(fec_decoder.release(), allocator_);

        fec_parser_.reset(new (allocator_) rtp::Parser(format_map, NULL), allocator_);
        if (!fec_parser_) {
            return;
        }

        fec_reader_.reset(new (allocator_) fec::Reader(
                              config.fec, *fec_decoder_, *preader, *repair_queue_,
                              *fec_parser_, packet_pool, allocator_),
                          allocator_);
        if (!fec_reader_ || !fec_reader_->valid()) {
            return;
        }
        preader = fec_reader_.get();

        fec_validator_.reset(new (allocator_)
                                 rtp::Validator(*preader, *format, config.rtp_validator),
                             allocator_);
        if (!fec_validator_) {
            return;
        }
        preader = fec_validator_.get();
    }
#endif // ROC_TARGET_OPENFEC

    decoder_.reset(format->new_decoder(allocator_), allocator_);
    if (!decoder_) {
        return;
    }

    depacketizer_.reset(new (allocator_) audio::Depacketizer(
                            *preader, *decoder_, config.channels, config.beeping),
                        allocator_);
    if (!depacketizer_) {
        return;
    }

    audio::IReader* areader = depacketizer_.get();

    if (config.watchdog.silence_timeout != 0 || config.watchdog.drops_timeout != 0
        || config.watchdog.frame_status_window != 0) {
        watchdog_.reset(new (allocator_) audio::Watchdog(
                            *areader, packet::num_channels(config.channels),
                            config.watchdog, allocator_),
                        allocator_);
        if (!watchdog_ || !watchdog_->valid()) {
            return;
        }
        areader = watchdog_.get();
    }

    if (config.resampling) {
        resampler_.reset(new (allocator_) audio::ResamplerReader(
                             *areader, sample_buffer_pool, allocator, config.resampler,
                             config.channels),
                         allocator_);
        if (!resampler_ || !resampler_->valid()) {
            return;
        }
        areader = resampler_.get();
    }

    latency_monitor_.reset(new (allocator_) audio::LatencyMonitor(
                               *source_queue_, *depacketizer_, resampler_.get(),
                               config.latency_monitor, config.latency,
                               format->sample_rate, out_sample_rate),
                           allocator_);
    if (!latency_monitor_ || !latency_monitor_->valid()) {
        return;
    }

    audio_reader_ = areader;
}

void ReceiverSession::destroy() {
    allocator_.destroy(*this);
}

bool ReceiverSession::valid() const {
    return audio_reader_;
}

bool ReceiverSession::handle(const packet::PacketPtr& packet) {
    roc_panic_if(!valid());

    packet::UDP* udp = packet->udp();
    if (!udp) {
        return false;
    }

    if (udp->src_addr != src_address_) {
        return false;
    }

    queue_router_->write(packet);
    return true;
}

bool ReceiverSession::update(packet::timestamp_t time) {
    roc_panic_if(!valid());

    if (watchdog_) {
        if (!watchdog_->update(time)) {
            return false;
        }
    }

    if (latency_monitor_) {
        if (!latency_monitor_->update(time)) {
            return false;
        }
    }

    return true;
}

audio::IReader& ReceiverSession::reader() {
    roc_panic_if(!valid());

    return *audio_reader_;
}

} // namespace pipeline
} // namespace roc
