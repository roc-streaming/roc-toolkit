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

#include "roc_core/buffer_factory.h"
#include "roc_core/iallocator.h"
#include "roc_core/mutex.h"
#include "roc_core/optional.h"
#include "roc_core/stddefs.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/pipeline_loop.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Receiver pipeline loop.
//!
//! This class acts as a task-based facade for the receiver pipeline subsystem
//! of roc_pipeline module (ReceiverSource, ReceiverEndpointSet, ReceiverEndpoint,
//! ReceiverSessionGroup, ReceiverSession).
//!
//! It provides two interfaces:
//!
//!  - sndio::ISource - can be used to retrieve samples from the pipeline
//!    (should be used from sndio thread)
//!
//!  - PipelineLoop - can be used to schedule tasks on the pipeline
//!    (can be used from any thread)
class ReceiverLoop : public PipelineLoop, private sndio::ISource {
public:
    //! Opaque endpoint set handle.
    typedef struct EndpointSetHandle* EndpointSetHandle;

    //! Base task class.
    class Task : public PipelineTask {
    protected:
        friend class ReceiverLoop;

        Task();

        bool (ReceiverLoop::*func_)(Task&); //!< Task implementation method.

        ReceiverEndpointSet* endpoint_set_; //!< Endpoint set.
        address::Interface iface_;          //!< Interface.
        address::Protocol proto_;           //!< Protocol.
        packet::IWriter* writer_;           //!< Packet writer.
    };

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Add new endpoint set.
        class CreateEndpointSet : public Task {
        public:
            //! Set task parameters.
            CreateEndpointSet();

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
    ReceiverLoop(IPipelineTaskScheduler& scheduler,
                 const ReceiverConfig& config,
                 const rtp::FormatMap& format_map,
                 packet::PacketFactory& packet_factory,
                 core::BufferFactory<uint8_t>& byte_buffer_factory,
                 core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                 core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Get receiver sources.
    //! @remarks
    //!  Samples received from remote peers become available in this source.
    sndio::ISource& source();

private:
    virtual size_t sample_rate() const;
    virtual size_t num_channels() const;
    virtual bool has_clock() const;
    virtual State state() const;
    virtual void pause();
    virtual bool resume();
    virtual bool restart();
    virtual bool read(audio::Frame&);

    virtual core::nanoseconds_t timestamp_imp() const;
    virtual bool process_subframe_imp(audio::Frame& frame);
    virtual bool process_task_imp(PipelineTask& task);

    bool task_create_endpoint_set_(Task& task);
    bool task_create_endpoint_(Task& task);
    bool task_delete_endpoint_(Task& task);

    ReceiverSource source_;

    core::Optional<core::Ticker> ticker_;
    packet::timestamp_t timestamp_;

    core::Mutex read_mutex_;

    bool valid_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_LOOP_H_
