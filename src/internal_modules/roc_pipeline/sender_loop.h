/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_loop.h
//! @brief Sender pipeline loop.

#ifndef ROC_PIPELINE_SENDER_LOOP_H_
#define ROC_PIPELINE_SENDER_LOOP_H_

#include "roc_core/buffer_factory.h"
#include "roc_core/iallocator.h"
#include "roc_core/mutex.h"
#include "roc_core/ticker.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/pipeline_loop.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Sender pipeline loop.
//
//! This class acts as a task-based facade for the sender pipeline subsystem
//! of roc_pipeline module (SenderSink, SenderEndpointSet, SenderEndpoint).
//!
//! It provides two interfaces:
//!
//!  - sndio::ISink - can be used to pass samples to the pipeline
//!    (should be used from sndio thread)
//!
//!  - PipelineLoop - can be used to schedule tasks on the pipeline
//!    (can be used from any thread)
class SenderLoop : public PipelineLoop, private sndio::ISink {
public:
    //! Opaque endpoint set handle.
    typedef struct EndpointSetHandle* EndpointSetHandle;

    //! Opaque endpoint handle.
    typedef struct EndpointHandle* EndpointHandle;

    //! Base task class.
    class Task : public PipelineTask {
    protected:
        friend class SenderLoop;

        Task();

        bool (SenderLoop::*func_)(Task&); //!< Task implementation method.

        SenderEndpointSet* endpoint_set_; //!< Endpoint set.
        SenderEndpoint* endpoint_;        //!< Endpoint.
        address::Interface iface_;        //!< Interface.
        address::Protocol proto_;         //!< Protocol.
        packet::IWriter* writer_;         //!< Packet writer.
        address::SocketAddr addr_;        //!< Endpoint address.
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

            //! Get created endpoint handle.
            EndpointHandle get_handle() const;
        };

        //! Set writer to which endpoint will write packets.
        class SetEndpointDestinationWriter : public Task {
        public:
            //! Set task parameters.
            SetEndpointDestinationWriter(EndpointHandle endpoint,
                                         packet::IWriter& writer);
        };

        //! Set UDP address for output packets of endpoint.
        class SetEndpointDestinationAddress : public Task {
        public:
            //! Set task parameters.
            SetEndpointDestinationAddress(EndpointHandle endpoint,
                                          const address::SocketAddr& addr);
        };

        //! Check if the endpoint set configuration is done.
        //! This is true when all necessary endpoints are added and configured.
        class CheckEndpointSetIsReady : public Task {
        public:
            //! Set task parameters.
            CheckEndpointSetIsReady(EndpointSetHandle endpoint_set);
        };
    };

    //! Initialize.
    SenderLoop(IPipelineTaskScheduler& scheduler,
               const SenderConfig& config,
               const rtp::FormatMap& format_map,
               packet::PacketFactory& packet_factory,
               core::BufferFactory<uint8_t>& byte_buffer_factory,
               core::BufferFactory<audio::sample_t>& sample_buffer_factory,
               core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Get sender sink.
    //! @remarks
    //!  Samples written to the sink are sent to remote peers.
    sndio::ISink& sink();

private:
    // Methods of sndio::ISink
    virtual size_t sample_rate() const;
    virtual size_t num_channels() const;
    virtual size_t latency() const;
    virtual bool has_clock() const;
    virtual void write(audio::Frame& frame);

    // Methods of PipelineLoop
    virtual core::nanoseconds_t timestamp_imp() const;
    virtual bool process_subframe_imp(audio::Frame&);
    virtual bool process_task_imp(PipelineTask&);

    // Methods for tasks
    bool task_create_endpoint_set_(Task&);
    bool task_create_endpoint_(Task&);
    bool task_set_endpoint_destination_writer_(Task&);
    bool task_set_endpoint_destination_address_(Task&);
    bool task_check_endpoint_set_is_ready_(Task&);

    SenderSink sink_;

    core::Optional<core::Ticker> ticker_;
    packet::timestamp_t timestamp_;

    core::Mutex write_mutex_;

    bool valid_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_LOOP_H_
