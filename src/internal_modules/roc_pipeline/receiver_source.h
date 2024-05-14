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

#include "roc_audio/iframe_reader.h"
#include "roc_audio/mixer.h"
#include "roc_audio/pcm_mapper_reader.h"
#include "roc_audio/profiling_reader.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/iarena.h"
#include "roc_core/optional.h"
#include "roc_core/stddefs.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_slot.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_rtp/encoding_map.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Receiver source pipeline.
//!
//! Contains:
//!  - one or more receiver slots
//!  - mixer, to mix audio from all slots
//!
//! Pipeline:
//!  - input: packets
//!  - output: frames
class ReceiverSource : public sndio::ISource, public core::NonCopyable<> {
public:
    //! Initialize.
    ReceiverSource(const ReceiverSourceConfig& source_config,
                   const rtp::EncodingMap& encoding_map,
                   packet::PacketFactory& packet_factory,
                   core::BufferFactory& byte_buffer_factory,
                   core::BufferFactory& sample_buffer_factory,
                   core::IArena& arena);

    //! Check if the pipeline was successfully constructed.
    bool is_valid() const;

    //! Create slot.
    ReceiverSlot* create_slot(const ReceiverSlotConfig& slot_config);

    //! Delete slot.
    void delete_slot(ReceiverSlot* slot);

    //! Get number of active sessions.
    size_t num_sessions() const;

    //! Pull packets and refresh pipeline according to current time.
    //! @remarks
    //!  Should be invoked before reading each frame.
    //!  Also should be invoked after provided deadline if no frames were
    //!  read until that deadline expires.
    //! @returns
    //!  deadline (absolute time) when refresh should be invoked again
    //!  if there are no frames
    core::nanoseconds_t refresh(core::nanoseconds_t current_time);

    //! Cast IDevice to ISink.
    virtual sndio::ISink* to_sink();

    //! Cast IDevice to ISink.
    virtual sndio::ISource* to_source();

    //! Get device type.
    virtual sndio::DeviceType type() const;

    //! Get current receiver state.
    virtual sndio::DeviceState state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Get latency of the source.
    virtual core::nanoseconds_t latency() const;

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Adjust sessions clock to match consumer clock.
    //! @remarks
    //!  @p playback_time specified absolute time when first sample of last frame
    //!  retrieved from pipeline will be actually played on sink
    virtual void reclock(core::nanoseconds_t playback_time);

    //! Read audio frame.
    virtual bool read(audio::Frame&);

private:
    ReceiverSourceConfig source_config_;

    const rtp::EncodingMap& encoding_map_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory& byte_buffer_factory_;
    core::BufferFactory& sample_buffer_factory_;
    core::IArena& arena_;

    StateTracker state_tracker_;

    core::Optional<audio::Mixer> mixer_;
    core::Optional<audio::ProfilingReader> profiler_;
    core::Optional<audio::PcmMapperReader> pcm_mapper_;

    core::List<ReceiverSlot> slots_;

    audio::IFrameReader* frame_reader_;

    bool valid_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SOURCE_H_
