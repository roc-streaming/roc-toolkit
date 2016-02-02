/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_config/config.h
//! @brief Compile-time configuration.

#ifndef ROC_CONFIG_CONFIG_H_
#define ROC_CONFIG_CONFIG_H_

//! Maximum payload size for UDP datagram.
#define ROC_CONFIG_MAX_UDP_BUFSZ 1472

//! Maximum number of sending/receiving ports.
#define ROC_CONFIG_MAX_PORTS 32

//! Maximum number of connected sessions.
#define ROC_CONFIG_MAX_SESSIONS 10

//! Maximum number of different queues per session.
#define ROC_CONFIG_MAX_SESSION_QUEUES 5

//! Maximum number of packets per queue of one session.
#define ROC_CONFIG_MAX_SESSION_PACKETS 1000

//! Maximum number of datagrams.
#define ROC_CONFIG_MAX_DATAGRAMS                                                         \
    (ROC_CONFIG_MAX_SESSIONS * ROC_CONFIG_MAX_SESSION_QUEUES                             \
     * ROC_CONFIG_MAX_SESSION_PACKETS)

//! Maximum number of sample buffers.
#define ROC_CONFIG_MAX_SAMPLE_BUFFERS 100

//! Maximum number of audio channels.
#define ROC_CONFIG_MAX_CHANNELS 2

//! Maximum allowed delta between two consecutive packets' seqnums.
#define ROC_CONFIG_MAX_SN_JUMP 100

//! Maximum allowed delta between two consecutive packets' timestamps.
#define ROC_CONFIG_MAX_TS_JUMP                                                           \
    (ROC_CONFIG_MAX_SN_JUMP * ROC_CONFIG_DEFAULT_PACKET_SAMPLES)

//! Bitmask of enabled audio channels.
#define ROC_CONFIG_DEFAULT_CHANNEL_MASK 0x3

//! Number of audio samples per second.
#define ROC_CONFIG_DEFAULT_SAMPLE_RATE 44100

//! Number of audio samples per packet (per channel).
#define ROC_CONFIG_DEFAULT_PACKET_SAMPLES 320

//! Number of audio samples per rendering tick (per channel).
#define ROC_CONFIG_DEFAULT_RECEIVER_TICK_SAMPLES 256

//! Number of audio sampler per resampler frame.
//! @remarks
//!  Allowed values are limited with resampler implementation. This parameter
//!  affects minimum required latency and maximum allowed scaling ratio. It
//!  doesn't affect allowed packet size or tick size.
#define ROC_CONFIG_DEFAULT_RESAMPLER_FRAME_SAMPLES 96

//! Session timeout (number of samples per channel).
#define ROC_CONFIG_DEFAULT_SESSION_TIMEOUT                                               \
    (ROC_CONFIG_DEFAULT_RECEIVER_TICK_SAMPLES * 100)

//! Latency audio renderer (samples per channel).
#define ROC_CONFIG_DEFAULT_SESSION_LATENCY (ROC_CONFIG_DEFAULT_PACKET_SAMPLES * 27)

//! Latency for audio output (samples per channel).
#define ROC_CONFIG_DEFAULT_OUTPUT_LATENCY (ROC_CONFIG_DEFAULT_PACKET_SAMPLES * 20)

//! Audio packet size including header.
#define ROC_CONFIG_DEFAULT_PACKET_SIZE (ROC_CONFIG_DEFAULT_PACKET_SAMPLES * 4 + 12)

//! Number of data packets in block.
#define ROC_CONFIG_DEFAULT_FEC_BLOCK_DATA_PACKETS 20

//! Number of FEC packets in block.
#define ROC_CONFIG_DEFAULT_FEC_BLOCK_REDUNDANT_PACKETS 10

//! Maximal number of data packets in block.
#define ROC_CONFIG_MAX_FEC_BLOCK_DATA_PACKETS 100

//! Maximal number of FEC packets in block.
#define ROC_CONFIG_MAX_FEC_BLOCK_REDUNDANT_PACKETS 100

#endif // ROC_CONFIG_CONFIG_H_
