/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_source.h
//! @brief Receiver source pipeline.

#ifndef ROC_PIPELINE_RECEIVER_SOURCE_H_
#define ROC_PIPELINE_RECEIVER_SOURCE_H_

#include "roc_audio/ireader.h"
#include "roc_audio/mixer.h"
#include "roc_audio/poison_reader.h"
#include "roc_audio/profiling_reader.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/iallocator.h"
#include "roc_core/mutex.h"
#include "roc_core/optional.h"
#include "roc_core/stddefs.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_endpoint_set.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtp/format_map.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Receiver source pipeline.
//! @remarks
//!  - input: packets
//!  - output: frames
class ReceiverSource : public sndio::ISource, public core::NonCopyable<> {
public:
    //! Initialize.
    ReceiverSource(const ReceiverConfig& config,
                   const rtp::FormatMap& format_map,
                   packet::PacketFactory& packet_factory,
                   core::BufferFactory<uint8_t>& byte_buffer_factory,
                   core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                   core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Create endpoint set.
    ReceiverEndpointSet* create_endpoint_set();

    //! Get number of connected sessions.
    size_t num_sessions() const;

    //! Get source sample rate.
    virtual size_t sample_rate() const;

    //! Get number of channels for the source.
    virtual size_t num_channels() const;

    //! Get latency of the source, in number of samples per channel.
    virtual size_t latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Get current receiver state.
    virtual State state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Adjust source clock to match consumer clock.
    virtual void reclock(packet::ntp_timestamp_t timestamp);

    //! Read audio frame.
    virtual bool read(audio::Frame&);

private:
    const rtp::FormatMap& format_map_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& byte_buffer_factory_;
    core::BufferFactory<audio::sample_t>& sample_buffer_factory_;
    core::IAllocator& allocator_;

    ReceiverState receiver_state_;
    core::List<ReceiverEndpointSet> endpoint_sets_;

    core::Optional<audio::Mixer> mixer_;
    core::Optional<audio::PoisonReader> poisoner_;
    core::Optional<audio::ProfilingReader> profiler_;

    audio::IReader* audio_reader_;

    ReceiverConfig config_;

    packet::timestamp_t timestamp_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SOURCE_H_
