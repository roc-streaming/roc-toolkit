/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/config.h
 * @brief Configuration options.
 */

#ifndef ROC_CONFIG_H_
#define ROC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Network protocol. */
typedef enum roc_protocol {
    /** Bare RTP (RFC 3550). */
    ROC_PROTO_RTP = 1,

    /** RTP source packet (RFC 3550) + FECFRAME Reed-Solomon footer (RFC 6865) with m=8. */
    ROC_PROTO_RTP_RSM8_SOURCE = 2,

    /** FEC repair packet + FECFRAME Reed-Solomon header (RFC 6865) with m=8. */
    ROC_PROTO_RSM8_REPAIR = 3,

    /** RTP source packet (RFC 3550) + FECFRAME LDPC-Staircase footer (RFC 6816). */
    ROC_PROTO_RTP_LDPC_SOURCE = 4,

    /** FEC repair packet + FECFRAME LDPC-Staircase header (RFC 6816). */
    ROC_PROTO_LDPC_REPAIR = 5
} roc_protocol;

/** Forward Error Correction scheme. */
typedef enum roc_fec_scheme {
    /** No FEC scheme.
     * Compatible with ROC_PROTO_RTP protocol.
     */
    ROC_FEC_DISABLE = -1,

    /** Default FEC scheme.
     * Current default is ROC_FEC_RS8M.
     */
    ROC_FEC_DEFAULT = 0,

    /** Reed-Solomon FEC scheme (RFC 6865) with m=8.
     * Good for small block sizes (below 256 packets).
     * Compatible with ROC_PROTO_RTP_RSM8_SOURCE and ROC_PROTO_RSM8_REPAIR
     * protocols for source and repair ports.
     */
    ROC_FEC_RS8M = 1,

    /** LDPC-Staircase FEC scheme (RFC 6816).
     * Good for large block sizes (above 1024 packets).
     * Compatible with ROC_PROTO_RTP_LDPC_SOURCE and ROC_PROTO_LDPC_REPAIR
     * protocols for source and repair ports.
     */
    ROC_FEC_LDPC_STAIRCASE = 2
} roc_fec_scheme;

/** Resampler profile. */
typedef enum roc_resampler_profile {
    /** No resampling. */
    ROC_RESAMPLER_DISABLE = -1,

    /** Default profile.
     * Current default is ROC_RESAMPLER_MEDIUM.
     */
    ROC_RESAMPLER_DEFAULT = 0,

    /** Hight quality, low speed. */
    ROC_RESAMPLER_HIGH = 1,

    /** Medium quality, medium speed. */
    ROC_RESAMPLER_MEDIUM = 2,

    /** Low quality, high speed. */
    ROC_RESAMPLER_LOW = 3
} roc_resampler_profile;

/** Context configuration.
 * Any field may be set to zero to use its default value.
 * @see roc_context
 */
typedef struct roc_context_config {
    /** Maximum size in bytes of a network packet.
     * Defines the amount of bytes allocated per network packet.
     * Sender and receiver won't handle packets larger than this.
     */
    unsigned int max_packet_size;

    /** Maximum size in bytes of an audio frame.
     * Defines the amount of bytes allocated per intermediate frame in the pipeline.
     * Does not limit the size of the frames provided by user.
     */
    unsigned int max_frame_size;
} roc_context_config;

/** Sender configuration.
 * Any field may be set to zero to use its default value.
 * @see roc_sender
 */
typedef struct roc_sender_config {
    /** The rate of the samples written to the sender by user.
     * Number of samples per channel per second.
     * May differ from the rate of the samples sent to the network,
     * but this requires resampler to be enabled.
     */
    unsigned int input_sample_rate;

    /** Enable automatic timing.
     * If non-zero, the sender write operation restricts the write rate according
     * to the input_sample_rate parameter. If zero, no restrictions are applied.
     */
    unsigned int automatic_timing;

    /** Resampler profile to use.
     * If non-zero, the sender employs resampler if the input sample rate differs
     * from the network sample rate.
     */
    roc_resampler_profile resampler_profile;

    /** The size of the packets produced by sender.
     * Number of samples per channel per packet. If zero, default value is used.
     * The samples written to the sender are buffered until the full packet is
     * accumulated or the sender is flushed or closed. Larger number reduces
     * packet overhead but also increases latency.
     */
    unsigned int packet_samples;

    /** FEC scheme to use.
     * If non-zero, the sender employs FEC codec to generate redundant packets
     * which may be used on receiver to restore dropped packets. This requires
     * both sender and receiver to use two separate source and repair ports.
     */
    roc_fec_scheme fec_scheme;

    /** Number of source packets per FEC block.
     * Used if some FEC scheme is selected. If zero, default value is used.
     * Larger number increases robustness but also increases latency.
     */
    unsigned int fec_block_source_packets;

    /** Number of repair packets per FEC block.
     * Used if some FEC scheme is selected. If zero, default value is used.
     * Larger number increases robustness but also increases traffic.
     */
    unsigned int fec_block_repair_packets;

    /** Enable packet interleaving.
     * If non-zero, the sender shuffles packets before sending them. This
     * may increase robustness but also increases latency.
     */
    unsigned int packet_interleaving;
} roc_sender_config;

/** Receiver configuration.
 * Any field may be set to zero to use its default value.
 * @see roc_receiver
 */
typedef struct roc_receiver_config {
    /** The rate of the samples returned to the user by receiver.
     * Number of samples per channel per second.
     * May differ from the rate of the samples received from the network,
     * but this requires resampler to be enabled.
     */
    unsigned int output_sample_rate;

    /** Enable automatic timing.
     * If non-zero, the receiver read operation restricts the read rate according
     * to the output_sample_rate parameter. If zero, no restrictions are applied.
     */
    unsigned int automatic_timing;

    /** Resampler profile to use.
     * If non-zero, the receiver employs resampler for two purposes:
     *  - adjust the sender clock to the receiver clock, which may differ a bit
     *  - convert the network sample rate to the output sample rate, if necessary
     */
    roc_resampler_profile resampler_profile;

    /** Target latency, in samples.
     * TODO
     */
    unsigned int target_latency;

    /** Silence timeout, in samples.
     * TODO
     */
    unsigned int silence_timeout;

    /** The size of the packets received from sender.
     * Number of samples per channel per packet. If zero, default value is used.
     * Should be set to the same value as on the sender.
     */
    unsigned int packet_samples;

    /** FEC scheme to use.
     * If non-zero, the receiver employs FEC codec to restore dropped packets.
     * This requires both sender and receiver to use two separate source and
     * repair ports. Should be set to the same value as on the sender.
     */
    roc_fec_scheme fec_scheme;

    /** Number of source packets per FEC block.
     * Used if some FEC scheme is selected. If zero, default value is used.
     * Should be set to the same value as on the sender.
     */
    unsigned int fec_block_source_packets;

    /** Number of repair packets per FEC block.
     * Used if some FEC scheme is selected. If zero, default value is used.
     * Should be set to the same value as on the sender.
     */
    unsigned int fec_block_repair_packets;
} roc_receiver_config;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_CONFIG_H_
