/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc/config.h
//! @brief Roc configuration.

#ifndef ROC_CONFIG_H_
#define ROC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//! Network protocol.
typedef enum roc_protocol {
    //! Bare RTP.
    ROC_PROTO_RTP = 0,

    //! RTP source packet + FECFRAME Reed-Solomon footer (m=8).
    ROC_PROTO_RTP_RSM8_SOURCE = 1,

    //! FEC repair packet + FECFRAME Reed-Solomon header (m=8).
    ROC_PROTO_RSM8_REPAIR = 2,

    //! RTP source packet + FECFRAME LDPC footer.
    ROC_PROTO_RTP_LDPC_SOURCE = 3,

    //! FEC repair packet + FECFRAME LDPC header.
    ROC_PROTO_LDPC_REPAIR = 4
} roc_protocol;

//! FEC scheme type.
typedef enum roc_fec_scheme {
    //! Reed-Solomon FEC code (m=8).
    //! Good for small block sizes (below 256 packets).
    ROC_FEC_RS8M = 0,

    //! LDPC-Staircase FEC code.
    //! Good for large block sizes (above 1024 packets).
    ROC_FEC_LDPC_STAIRCASE = 1,

    //! Disable FEC.
    ROC_FEC_DISABLE = 2
} roc_fec_scheme;

//! Resampler profile.
typedef enum roc_resampler_profile {
    //! Medium quality, medium speed.
    ROC_RESAMPLER_MEDIUM = 0,

    //! Hight quality, low speed.
    ROC_RESAMPLER_HIGH = 1,

    //! Low quality, fast speed.
    ROC_RESAMPLER_LOW = 2,

    //! Disable resampler.
    ROC_RESAMPLER_DISABLE = 3
} roc_resampler_profile;

//! Context configuration.
typedef struct roc_context_config {
    //! Maximum size in bytes of a network packet.
    unsigned int max_packet_size;

    //! Maximum size in bytes of an audio frame.
    unsigned int max_frame_size;
} roc_context_config;

//! Sender configuration.
typedef struct roc_sender_config {
    //! Number of samples per second per channel.
    unsigned int input_sample_rate;

    //! Constrain sending speed to the input sample rate using a CPU timer.
    unsigned int enable_timing;

    //! Resampler profile to use.
    roc_resampler_profile resampler_profile;

    //! Number of samples per channel per packet.
    unsigned int packet_size;

    //! FEC scheme to use.
    roc_fec_scheme fec_scheme;

    //! Number of source packets per FEC block.
    unsigned int fec_block_source_packets;

    //! Number of repair packets per FEC block.
    unsigned int fec_block_repair_packets;

    //! Shuffle packets before sending them to increase chances that
    //! missing packets will be reconstructed.
    unsigned int enable_interleaving;
} roc_sender_config;

//! Receiver configuration.
typedef struct roc_receiver_config {
    //! Number of samples per second per channel.
    unsigned int output_sample_rate;

    //! Constrain receiving speed to the output sample rate using a CPU timer.
    unsigned int enable_timing;

    //! Resampler profile to use.
    roc_resampler_profile resampler_profile;

    //! Session latency as number of samples.
    unsigned int target_latency;

    //! Timeout after which session is terminated as number of samples.
    unsigned int silence_timeout;

    //! Number of samples per channel per packet.
    unsigned int packet_size;

    //! FEC scheme to use.
    roc_fec_scheme fec_scheme;

    //! Number of source packets per FEC block.
    unsigned int fec_block_source_packets;

    //! Number of repair packets per FEC block.
    unsigned int fec_block_repair_packets;
} roc_receiver_config;

#ifdef __cplusplus
}
#endif

#endif // ROC_CONFIG_H_
