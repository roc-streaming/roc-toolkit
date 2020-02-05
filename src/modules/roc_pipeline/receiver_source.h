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
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_endpoint_set.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtp/format_map.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Receiver source pipeline.
//! Thread-safe.
//! @remarks
//!  - input: packets
//!  - output: frames
class ReceiverSource : public sndio::ISource, public core::NonCopyable<> {
public:
    //! Initialize.
    ReceiverSource(const ReceiverConfig& config,
                   const rtp::FormatMap& format_map,
                   packet::PacketPool& packet_pool,
                   core::BufferPool<uint8_t>& byte_buffer_pool,
                   core::BufferPool<audio::sample_t>& sample_buffer_pool,
                   core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Opaque endpoint set handle.
    typedef struct EndpointSetHandle* EndpointSetHandle;

    //! Add new endpoint set.
    //! @returns
    //!  NULL on on error or non-NULL opaque handle on success.
    EndpointSetHandle add_endpoint_set();

    //! Add endpoint to endpoint set.
    //! Each endpoint set can have one source and zero or one repair endpoint.
    //! The protocols of endpoints in one set should be compatible.
    //! @returns
    //!  thread-safe packet writer for the newly created endpoint or NULL on error.
    packet::IWriter* add_endpoint(EndpointSetHandle endpoint_set,
                                  address::Interface iface,
                                  address::Protocol proto);

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
    virtual bool read(audio::Frame&);

private:
    void update_(packet::timestamp_t timestamp);

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

    audio::IReader* audio_reader_;

    ReceiverConfig config_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;

    core::Mutex mutex_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SOURCE_H_
