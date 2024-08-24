/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_sink.h
//! @brief Sender sink pipeline.

#ifndef ROC_PIPELINE_SENDER_SINK_H_
#define ROC_PIPELINE_SENDER_SINK_H_

#include "roc_audio/fanout.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/pcm_mapper_writer.h"
#include "roc_audio/processor_map.h"
#include "roc_audio/profiling_writer.h"
#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_dbgio/csv_dumper.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_pipeline/sender_slot.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_rtp/encoding_map.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Sender sink pipeline.
//!
//! Contains:
//!  - one or more sender slots
//!  - fanout, to duplicate audio to all slots
//!
//! Pipeline:
//!  - input: frames
//!  - output: packets
class SenderSink : public sndio::ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    SenderSink(const SenderSinkConfig& sink_config,
               audio::ProcessorMap& processor_map,
               rtp::EncodingMap& encoding_map,
               core::IPool& packet_pool,
               core::IPool& packet_buffer_pool,
               core::IPool& frame_pool,
               core::IPool& frame_buffer_pool,
               core::IArena& arena);
    ~SenderSink();

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Create slot.
    SenderSlot* create_slot(const SenderSlotConfig& slot_config);

    //! Delete slot.
    void delete_slot(SenderSlot* slot);

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

    //! Get sample specification of the sink.
    virtual audio::SampleSpec sample_spec() const;

    //! Get recommended frame length of the sink.
    virtual core::nanoseconds_t frame_length() const;

    //! Check if the sink supports state updates.
    virtual bool has_state() const;

    //! Get current sink state.
    virtual sndio::DeviceState state() const;

    //! Pause sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode pause();

    //! Resume sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode resume();

    //! Check if the sink supports latency reports.
    virtual bool has_latency() const;

    //! Get latency of the sink.
    virtual core::nanoseconds_t latency() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(audio::Frame& frame);

    //! Flush buffered data, if any.
    virtual ROC_ATTR_NODISCARD status::StatusCode flush();

    //! Explicitly close the sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Destroy object and return memory to arena.
    virtual void dispose();

private:
    SenderSinkConfig sink_config_;

    audio::ProcessorMap& processor_map_;
    rtp::EncodingMap& encoding_map_;

    packet::PacketFactory packet_factory_;
    audio::FrameFactory frame_factory_;
    core::IArena& arena_;

    StateTracker state_tracker_;

    core::Optional<dbgio::CsvDumper> dumper_;

    core::Optional<audio::Fanout> fanout_;
    core::Optional<audio::ProfilingWriter> profiler_;
    core::Optional<audio::PcmMapperWriter> pcm_mapper_;

    core::List<SenderSlot> slots_;

    audio::IFrameWriter* frame_writer_;

    status::StatusCode init_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SINK_H_
