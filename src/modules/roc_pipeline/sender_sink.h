/*
 * Copyright (c) 2017 Roc authors
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
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/poison_writer.h"
#include "roc_audio/profiling_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/ticker.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/sender_endpoint_set.h"
#include "roc_pipeline/task_pipeline.h"
#include "roc_rtp/format_map.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Sender sink pipeline.
//! Thread-safe.
//! @remarks
//!  - input: frames
//!  - output: packets
class SenderSink : public sndio::ISink, public TaskPipeline {
public:
    //! Opaque endpoint set handle.
    typedef struct EndpointSetHandle* EndpointSetHandle;

    //! Opaque endpoint handle.
    typedef struct EndpointHandle* EndpointHandle;

    //! Base task class.
    //! The user is responsible for allocating and deallocating the task.
    class Task : public TaskPipeline::Task {
    protected:
        friend class SenderSink;

        Task();

        bool (SenderSink::*func_)(Task&); //!< Task implementation method.

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

            //! Get created endpoint handle.
            EndpointHandle get_handle() const;
        };

        //! Set writer to which endpoint will write packets.
        class SetEndpointOutputWriter : public Task {
        public:
            //! Set task parameters.
            SetEndpointOutputWriter(EndpointHandle endpoint, packet::IWriter& writer);
        };

        //! Set UDP address for output packets.
        class SetEndpointDestinationUdpAddress : public Task {
        public:
            //! Set task parameters.
            SetEndpointDestinationUdpAddress(EndpointHandle endpoint,
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
    SenderSink(ITaskScheduler& scheduler,
               const SenderConfig& config,
               const rtp::FormatMap& format_map,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& byte_buffer_pool,
               core::BufferPool<audio::sample_t>& sample_buffer_pool,
               core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Get sink sample rate.
    virtual size_t sample_rate() const;

    //! Get number of channels for the sink.
    virtual size_t num_channels() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    virtual core::nanoseconds_t timestamp_imp() const;

    virtual bool process_frame_imp(audio::Frame&);
    virtual bool process_task_imp(TaskPipeline::Task&);

    bool task_add_endpoint_set_(Task&);
    bool task_create_endpoint_(Task&);
    bool task_set_endpoint_output_writer_(Task&);
    bool task_set_endpoint_destination_udp_address_(Task&);
    bool task_check_endpoint_set_is_ready_(Task&);

    const SenderConfig config_;

    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;

    core::IAllocator& allocator_;

    core::List<SenderEndpointSet> endpoint_sets_;

    audio::Fanout fanout_;

    core::ScopedPtr<audio::PoisonWriter> pipeline_poisoner_;

    core::ScopedPtr<audio::ProfilingWriter> profiler_;

    core::ScopedPtr<core::Ticker> ticker_;

    audio::IWriter* audio_writer_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;

    core::Mutex write_mutex_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SINK_H_
