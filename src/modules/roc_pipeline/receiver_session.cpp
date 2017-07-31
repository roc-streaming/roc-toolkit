/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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
                                 const packet::Address& src_address,
                                 const rtp::FormatMap& format_map,
                                 packet::PacketPool& packet_pool,
                                 core::BufferPool<uint8_t>& byte_buffer_pool,
                                 core::BufferPool<audio::sample_t>& sample_buffer_pool,
                                 core::IAllocator& allocator)
    : src_address_(src_address)
    , allocator_(allocator)
    , audio_reader_(NULL) {
    const rtp::Format* format = format_map.format(config.payload_type);
    if (!format) {
        return;
    }

    if (config.resampling) {
        resampler_updater_.reset(new (allocator_) audio::ResamplerUpdater(
                                     config.fe_update_interval, config.latency),
                                 allocator_);
        if (!resampler_updater_) {
            return;
        }
    }

    queue_router_.reset(new (allocator_) packet::Router(allocator_, 2), allocator_);
    if (!queue_router_) {
        return;
    }

    source_queue_.reset(new (allocator_) packet::SortedQueue(0), allocator_);
    if (!source_queue_) {
        return;
    }

    packet::IWriter* pwriter = source_queue_.get();

    if (config.resampling) {
        resampler_updater_->set_writer(*pwriter);
        pwriter = resampler_updater_.get();
    }

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

    validator_.reset(new (allocator_) rtp::Validator(*preader, *format, config.validator),
                     allocator_);
    if (!validator_) {
        return;
    }
    preader = validator_.get();

    watchdog_.reset(new (allocator_) packet::Watchdog(*preader, config.timeout),
                    allocator_);
    if (!watchdog_) {
        return;
    }
    preader = watchdog_.get();

#ifdef ROC_TARGET_OPENFEC
    if (config.fec.codec != fec::NoCodec) {
        repair_queue_.reset(new (allocator_) packet::SortedQueue(0), allocator_);
        if (!repair_queue_) {
            return;
        }
        if (!queue_router_->add_route(*repair_queue_, packet::Packet::FlagRepair)) {
            return;
        }

        fec_decoder_.reset(new (allocator_) fec::OFDecoder(
                               config.fec, format->size(config.samples_per_packet),
                               byte_buffer_pool, allocator_),
                           allocator_);
        if (!fec_decoder_) {
            return;
        }

        fec_parser_.reset(new (allocator_) rtp::Parser(format_map, NULL), allocator_);
        if (!fec_parser_) {
            return;
        }

        fec_reader_.reset(new (allocator_) fec::Reader(
                              config.fec, *fec_decoder_, *preader, *repair_queue_,
                              *fec_parser_, packet_pool, allocator_),
                          allocator_);
        if (!fec_reader_) {
            return;
        }
        preader = fec_reader_.get();

        fec_validator_.reset(new (allocator_)
                                 rtp::Validator(*preader, *format, config.validator),
                             allocator_);
        if (!fec_validator_) {
            return;
        }
        preader = fec_validator_.get();

        fec_watchdog_.reset(new (allocator_) packet::Watchdog(*preader, config.timeout),
                            allocator_);
        if (!fec_watchdog_) {
            return;
        }
        preader = fec_watchdog_.get();
    }
#endif // ROC_TARGET_OPENFEC

    if (config.resampling) {
        resampler_updater_->set_reader(*preader);
        preader = resampler_updater_.get();
    }

    decoder_.reset(format->new_decoder(allocator_), allocator_);
    if (!decoder_) {
        return;
    }

    depacketizer_.reset(new (allocator_) audio::Depacketizer(
                            *preader, *decoder_, config.channels, config.beep),
                        allocator_);
    if (!depacketizer_) {
        return;
    }

    audio::IReader* areader = depacketizer_.get();

    if (config.resampling) {
        resampler_.reset(new (allocator_)
                             audio::Resampler(*areader, sample_buffer_pool, allocator,
                                              config.resampler, config.channels),
                         allocator_);
        if (!resampler_) {
            return;
        }
        resampler_updater_->set_resampler(*resampler_);
        areader = resampler_.get();
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

    if (fec_watchdog_) {
        if (!fec_watchdog_->update(time)) {
            return false;
        }
    }

    if (resampler_updater_) {
        if (!resampler_updater_->update(time)) {
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
