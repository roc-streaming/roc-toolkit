/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_loop.h
//! @brief Receiver pipeline loop.

#ifndef ROC_PIPELINE_RECEIVER_LOOP_H_
#define ROC_PIPELINE_RECEIVER_LOOP_H_

#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/mutex.h"
#include "roc_core/optional.h"
#include "roc_core/stddefs.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/pipeline_loop.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Receiver pipeline loop.
//!
//! This class acts as a task-based facade for the receiver pipeline subsystem
//! of roc_pipeline module (ReceiverSource, ReceiverSlot, ReceiverEndpoint,
//! ReceiverSessionGroup, ReceiverSession).
//!
//! It provides two interfaces:
//!
//!  - sndio::ISource - can be used to retrieve samples from the pipeline
//!    (should be used from sndio thread)
//!
//!  - PipelineLoop - can be used to schedule tasks on the pipeline
//!    (can be used from any thread)
//!
//! @note
//!  Private inheritance from ISource is used to decorate actual implementation
//!  of ISource - ReceiverSource, in order to integrate it with PipelineLoop.
class ReceiverLoop : public PipelineLoop, private sndio::ISource {
public:
    //! Opaque slot handle.
    typedef struct SlotHandle* SlotHandle;

    //! Base task class.
    class Task : public PipelineTask {
    protected:
        friend class ReceiverLoop;

        Task();

        bool (ReceiverLoop::*func_)(Task&); //!< Task implementation method.

        ReceiverSlot* slot_;                        //!< Slot.
        ReceiverSlotConfig slot_config_;            //!< Slot config.
        address::Interface iface_;                  //!< Interface.
        address::Protocol proto_;                   //!< Protocol.
        address::SocketAddr inbound_address_;       //!< Inbound packet address.
        packet::IWriter* inbound_writer_;           //!< Inbound packet writer.
        packet::IWriter* outbound_writer_;          //!< Outbound packet writer.
        ReceiverSlotMetrics* slot_metrics_;         //!< Output slot metrics.
        ReceiverParticipantMetrics* party_metrics_; //!< Output participant metrics.
        size_t* party_count_;                       //!< Input/output participant count.
    };

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Create new slot.
        class CreateSlot : public Task {
        public:
            //! Set task parameters.
            CreateSlot(const ReceiverSlotConfig& slot_config);

            //! Get created slot handle.
            SlotHandle get_handle() const;
        };

        //! Delete existing slot.
        class DeleteSlot : public Task {
        public:
            //! Set task parameters.
            DeleteSlot(SlotHandle slot);
        };

        //! Query slot metrics.
        class QuerySlot : public Task {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Metrics are written to provided structs.
            QuerySlot(SlotHandle slot,
                      ReceiverSlotMetrics& slot_metrics,
                      ReceiverParticipantMetrics* party_metrics,
                      size_t* party_count);
        };

        //! Create endpoint on given interface of the slot.
        class AddEndpoint : public Task {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Each slot can have one source and zero or one repair endpoint.
            //!  The protocols of endpoints in one slot should be compatible.
            AddEndpoint(SlotHandle slot,
                        address::Interface iface,
                        address::Protocol proto,
                        const address::SocketAddr& inbound_address,
                        packet::IWriter* outbound_writer);

            //! Get packet writer for inbound packets for the endpoint.
            //! @remarks
            //!  The returned writer may be used from any thread.
            packet::IWriter* get_inbound_writer() const;
        };
    };

    //! Initialize.
    ReceiverLoop(IPipelineTaskScheduler& scheduler,
                 const ReceiverSourceConfig& source_config,
                 audio::ProcessorMap& processor_map,
                 rtp::EncodingMap& encoding_map,
                 core::IPool& packet_pool,
                 core::IPool& packet_buffer_pool,
                 core::IPool& frame_pool,
                 core::IPool& frame_buffer_pool,
                 core::IArena& arena);
    ~ReceiverLoop();

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Get receiver sources.
    //! @remarks
    //!  Samples received from remote peers become available in this source.
    sndio::ISource& source();

private:
    // Methods of sndio::ISource
    virtual sndio::DeviceType type() const;
    virtual sndio::ISink* to_sink();
    virtual sndio::ISource* to_source();
    virtual audio::SampleSpec sample_spec() const;
    virtual core::nanoseconds_t frame_length() const;
    virtual bool has_state() const;
    virtual sndio::DeviceState state() const;
    virtual status::StatusCode pause();
    virtual status::StatusCode resume();
    virtual bool has_latency() const;
    virtual core::nanoseconds_t latency() const;
    virtual bool has_clock() const;
    virtual status::StatusCode rewind();
    virtual void reclock(core::nanoseconds_t timestamp);
    virtual status::StatusCode read(audio::Frame& frame,
                                    packet::stream_timestamp_t duration,
                                    audio::FrameReadMode mode);
    virtual status::StatusCode close();
    virtual void dispose();

    // Methods of PipelineLoop
    virtual core::nanoseconds_t timestamp_imp() const;
    virtual uint64_t tid_imp() const;
    virtual status::StatusCode process_subframe_imp(audio::Frame& frame,
                                                    packet::stream_timestamp_t duration,
                                                    audio::FrameReadMode mode);
    virtual bool process_task_imp(PipelineTask& task);

    // Methods for tasks
    bool task_create_slot_(Task& task);
    bool task_delete_slot_(Task& task);
    bool task_query_slot_(Task& task);
    bool task_add_endpoint_(Task& task);

    ReceiverSource source_;
    core::Mutex source_mutex_;

    core::Optional<core::Ticker> ticker_;
    core::Ticker::ticks_t ticker_ts_;

    const bool auto_reclock_;

    status::StatusCode init_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_LOOP_H_
