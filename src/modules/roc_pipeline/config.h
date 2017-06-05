/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/config.h
//! @brief Client and server config.

#ifndef ROC_PIPELINE_CONFIG_H_
#define ROC_PIPELINE_CONFIG_H_

#include "roc_config/config.h"
#include "roc_core/ipool.h"
#include "roc_core/heap_pool.h"
#include "roc_datagram/default_buffer_composer.h"
#include "roc_packet/units.h"
#include "roc_fec/config.h"
#include "roc_audio/sample_buffer.h"
#include "roc_pipeline/session.h"

namespace roc {
namespace pipeline {

//! Server and client options.
enum Options {
    //! Use scaler and resamplers (server).
    EnableResampling = (1 << 0),

    //! Use interleaver (client).
    EnableInterleaving = (1 << 1),

    //! Constrain input/output speed (server, client).
    EnableTiming = (1 << 2),

    //! Insert beep instead of missing samples (server).
    EnableBeep = (1 << 3),

    //! Terminate server when first client disconects (server).
    EnableOneshot = (1 << 4)
};

//! Server config.
struct ServerConfig {
    //! Construct default config.
    ServerConfig(int opts = 0)
        : options(opts)
        , channels(ROC_CONFIG_DEFAULT_CHANNEL_MASK)
        , sample_rate(ROC_CONFIG_DEFAULT_SAMPLE_RATE)
        , samples_per_tick(ROC_CONFIG_DEFAULT_SERVER_TICK_SAMPLES)
        , samples_per_resampler_frame(ROC_CONFIG_DEFAULT_RESAMPLER_FRAME_SAMPLES)
        , output_latency(ROC_CONFIG_DEFAULT_OUTPUT_LATENCY)
        , session_latency(ROC_CONFIG_DEFAULT_SESSION_LATENCY)
        , session_timeout(ROC_CONFIG_DEFAULT_SESSION_TIMEOUT)
        , max_sessions(ROC_CONFIG_MAX_SESSIONS)
        , max_session_packets(ROC_CONFIG_MAX_SESSION_PACKETS)
        , byte_buffer_composer(&datagram::default_buffer_composer())
        , sample_buffer_composer(&audio::default_buffer_composer())
        , session_pool(&core::HeapPool<Session>::instance()) {
    }

    //! Bitmask of enabled session options.
    int options;

    //! Bitmask of enabled channels.
    packet::channel_mask_t channels;

    //! Number of samples per channel per second.
    size_t sample_rate;

    //! Number of samples per server tick.
    size_t samples_per_tick;

    //! Number of samples per resampler frame.
    size_t samples_per_resampler_frame;

    //! Output latency as number of samples.
    size_t output_latency;

    //! Session latency as number of samples.
    size_t session_latency;

    //! Timeout after which session is terminated as number of samples.
    size_t session_timeout;

    //! Maximum number of active sessions.
    size_t max_sessions;

    //! Maximum number of queued packets per session.
    size_t max_session_packets;

    //! Forward Error Correction code scheme configuration. FEC should
    //! be enabled by adding flag EnableFEC in the field \ref ClientConfig.options.
    fec::Config fec;

    //! Composer for byte buffers.
    core::IByteBufferComposer* byte_buffer_composer;

    //! Composer for sample buffers.
    audio::ISampleBufferComposer* sample_buffer_composer;

    //! Session pool.
    core::IPool<Session>* session_pool;
};

//! Client config.
struct ClientConfig {
    //! Construct default config.
    ClientConfig(int opts = 0)
        : options(opts)
        , channels(ROC_CONFIG_DEFAULT_CHANNEL_MASK)
        , sample_rate(ROC_CONFIG_DEFAULT_SAMPLE_RATE)
        , samples_per_packet(ROC_CONFIG_DEFAULT_PACKET_SAMPLES)
        , random_loss_rate(0)
        , random_delay_rate(0)
        , random_delay_time(0)
        , byte_buffer_composer(&datagram::default_buffer_composer()) {
    }

    //! Bitmask of enabled client options.
    int options;

    //! Bitmask of enabled channels.
    packet::channel_mask_t channels;

    //! Number of samples per channel per second.
    size_t sample_rate;

    //! Number of samples per channel per packet.
    size_t samples_per_packet;

    //! Percentage of packets to be lost in range [0; 100].
    size_t random_loss_rate;

    //! Percentage of packets to be delayed in range [0; 100].
    size_t random_delay_rate;

    //! Delay time in milliseconds.
    size_t random_delay_time;

    //! Forward Error Correction code scheme configuration. FEC should
    //! be enabled by adding flag EnableFEC in the field \ref ClientConfig.options.
    fec::Config fec;

    //! Composer for byte buffers.
    core::IByteBufferComposer* byte_buffer_composer;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
