/*
 * Copyright (c) 2017 Roc authors
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
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/mutex.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_endpoint_set.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_pipeline/task_pipeline.h"
#include "roc_rtp/format_map.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Receiver source pipeline.
//! Thread-safe.
//! @remarks
//!  - input: packets
//!  - output: frames
class ReceiverSource : public sndio::ISource, public TaskPipeline {
public:
    //! Opaque endpoint set handle.
    typedef struct EndpointSetHandle* EndpointSetHandle;

    //! Base task class.
    //! The user is responsible for allocating and deallocating the task.
    class Task : public TaskPipeline::Task {
    protected:
        friend class ReceiverSource;

        Task();

        bool (ReceiverSource::*func_)(Task&); //!< Task implementation method.

        ReceiverEndpointSet* endpoint_set_; //!< Endpoint set.
        address::Interface iface_;          //!< Interface.
        address::Protocol proto_;           //!< Protocol.
        packet::IWriter* writer_;           //!< Packet writer.
    };

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Add new endpoint set.
        class AddEndpointSet : public Task {
        public:
            //! Set task parameters.
            AddEndpointSet();

            //! Get created endpoint set handle.
            EndpointSetHandle get_handle() const;
        };

        //! Create endpoint on given interface of the endpoint set.
        class CreateEndpoint : public Task {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Each endpoint set can have one source and zero or one repair endpoint.
            //!  The protocols of endpoints in one set should be compatible.
            CreateEndpoint(EndpointSetHandle endpoint_set,
                           address::Interface iface,
                           address::Protocol proto);

            //! Get packet writer for the endpoint.
            //! @remarks
            //!  The returned writer may be used from any thread.
            packet::IWriter* get_writer() const;
        };

        //! Delete endpoint on given interface of the endpoint set, if it exists.
        class DeleteEndpoint : public Task {
        public:
            //! Set task parameters.
            DeleteEndpoint(EndpointSetHandle endpoint_set, address::Interface iface);
        };
    };

    //! Initialize.
    ReceiverSource(ITaskScheduler& scheduler,
                   const ReceiverConfig& config,
                   const rtp::FormatMap& format_map,
                   packet::PacketPool& packet_pool,
                   core::BufferPool<uint8_t>& byte_buffer_pool,
                   core::BufferPool<audio::sample_t>& sample_buffer_pool,
                   core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Get number of connected sessions.
    size_t num_sessions() const;

    //! Get source sample rate.
    virtual size_t sample_rate() const;

    //! Get number of channels for the source.
    virtual size_t num_channels() const;

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

    //! Read frame.
    //! May also process some tasks and invoke callbacks passed to enqueue().
    virtual bool read(audio::Frame&);

private:
    virtual core::nanoseconds_t timestamp_imp() const;

    virtual bool process_frame_imp(audio::Frame& frame);
    virtual bool process_task_imp(TaskPipeline::Task& task);

    bool task_add_endpoint_set_(Task& task);
    bool task_create_endpoint_(Task& task);
    bool task_delete_endpoint_(Task& task);

    core::Mutex read_mutex_;

    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;
    core::IAllocator& allocator_;

    ReceiverState receiver_state_;
    core::List<ReceiverEndpointSet> endpoint_sets_;

    core::Ticker ticker_;

    core::ScopedPtr<audio::Mixer> mixer_;
    core::ScopedPtr<audio::PoisonReader> poisoner_;
    core::ScopedPtr<audio::ProfilingReader> profiler_;

    audio::IReader* audio_reader_;

    ReceiverConfig config_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SOURCE_H_
