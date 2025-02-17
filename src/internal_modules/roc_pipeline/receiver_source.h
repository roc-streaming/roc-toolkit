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

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/mixer.h"
#include "roc_audio/pcm_mapper_reader.h"
#include "roc_audio/processor_map.h"
#include "roc_audio/profiling_reader.h"
#include "roc_core/iarena.h"
#include "roc_core/optional.h"
#include "roc_core/stddefs.h"
#include "roc_dbgio/csv_dumper.h"
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
                   audio::ProcessorMap& processor_map,
                   rtp::EncodingMap& encoding_map,
                   core::IPool& packet_pool,
                   core::IPool& packet_buffer_pool,
                   core::IPool& frame_pool,
                   core::IPool& frame_buffer_pool,
                   core::IArena& arena);

    ~ReceiverSource();

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Create slot.
    ReceiverSlot* create_slot(const ReceiverSlotConfig& slot_config);

    //! Delete slot.
    void delete_slot(ReceiverSlot* slot);

    //! Get number of active sessions.
    size_t num_sessions() const;

    //! Pull packets and refresh pipeline according to current time.
    //! @remarks
    //!  Should be invoked before reading each frame.
    //!  If there are no frames for a while, should be invoked no
    //!  later than the deadline returned via @p next_deadline.
    ROC_ATTR_NODISCARD status::StatusCode refresh(core::nanoseconds_t current_time,
                                                  core::nanoseconds_t* next_deadline);

    //! Get type (sink or source).
    virtual sndio::DeviceType type() const;

    //! Try to cast to ISink.
    virtual sndio::ISink* to_sink();

    //! Try to cast to ISource.
    virtual sndio::ISource* to_source();

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Get recommended frame length of the source.
    virtual core::nanoseconds_t frame_length() const;

    //! Check if the source supports state updates.
    virtual bool has_state() const;

    //! Get current source state.
    virtual sndio::DeviceState state() const;

    //! Pause source.
    virtual ROC_ATTR_NODISCARD status::StatusCode pause();

    //! Resume source.
    virtual ROC_ATTR_NODISCARD status::StatusCode resume();

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Get latency of the source.
    virtual core::nanoseconds_t latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Restart reading from beginning.
    virtual ROC_ATTR_NODISCARD status::StatusCode rewind();

    //! Adjust sessions clock to match consumer clock.
    //! @remarks
    //!  Should be invoked regularly after reading every or several frames.
    virtual void reclock(core::nanoseconds_t playback_time);

    //! Read frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(audio::Frame& frame,
         packet::stream_timestamp_t duration,
         audio::FrameReadMode mode);

    //! Explicitly close the source.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Destroy object and return memory to arena.
    virtual void dispose();

private:
    ReceiverSourceConfig source_config_;

    audio::ProcessorMap& processor_map_;
    rtp::EncodingMap& encoding_map_;

    packet::PacketFactory packet_factory_;
    audio::FrameFactory frame_factory_;
    core::IArena& arena_;

    StateTracker state_tracker_;

    core::Optional<dbgio::CsvDumper> dumper_;

    core::Optional<audio::Mixer> mixer_;
    core::Optional<audio::ProfilingReader> profiler_;
    core::Optional<audio::PcmMapperReader> pcm_mapper_;

    core::List<ReceiverSlot> slots_;

    audio::IFrameReader* frame_reader_;

    status::StatusCode init_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SOURCE_H_
