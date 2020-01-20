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

/** Network port type. */
typedef enum roc_port_type {
    /** Network port for audio source packets.
     * If FEC is not used, this type of port is used to send or receive audio packets.
     * If FEC is used, this type of port is used to send or receive FEC source packets
     * containing audio data plus some FEC headers.
     */
    ROC_PORT_AUDIO_SOURCE = 1,

    /** Network port for audio repair packets.
     * If FEC is used, this type of port is used to send or receive FEC repair packets
     * containing redundant data for audio plus some FEC headers.
     */
    ROC_PORT_AUDIO_REPAIR = 2
} roc_port_type;

/** Network protocol. */
typedef enum roc_protocol {
    /** Bare RTP (RFC 3550). */
    ROC_PROTO_RTP = 1,

    /** RTP source packet (RFC 3550) + FECFRAME Reed-Solomon footer (RFC 6865) with m=8.
     */
    ROC_PROTO_RTP_RS8M_SOURCE = 2,

    /** FEC repair packet + FECFRAME Reed-Solomon header (RFC 6865) with m=8. */
    ROC_PROTO_RS8M_REPAIR = 3,

    /** RTP source packet (RFC 3550) + FECFRAME LDPC-Staircase footer (RFC 6816). */
    ROC_PROTO_RTP_LDPC_SOURCE = 4,

    /** FEC repair packet + FECFRAME LDPC-Staircase header (RFC 6816). */
    ROC_PROTO_LDPC_REPAIR = 5
} roc_protocol;

/** Forward Error Correction code. */
typedef enum roc_fec_code {
    /** No FEC code.
     * Compatible with @c ROC_PROTO_RTP protocol.
     */
    ROC_FEC_DISABLE = -1,

    /** Default FEC code.
     * Current default is @c ROC_FEC_RS8M.
     */
    ROC_FEC_DEFAULT = 0,

    /** Reed-Solomon FEC code (RFC 6865) with m=8.
     * Good for small block sizes (below 256 packets).
     * Compatible with @c ROC_PROTO_RTP_RS8M_SOURCE and @c ROC_PROTO_RS8M_REPAIR
     * protocols for source and repair ports.
     */
    ROC_FEC_RS8M = 1,

    /** LDPC-Staircase FEC code (RFC 6816).
     * Good for large block sizes (above 1024 packets).
     * Compatible with @c ROC_PROTO_RTP_LDPC_SOURCE and @c ROC_PROTO_LDPC_REPAIR
     * protocols for source and repair ports.
     */
    ROC_FEC_LDPC_STAIRCASE = 2
} roc_fec_code;

/** Packet encoding. */
typedef enum roc_packet_encoding {
    /** PCM signed 16-bit.
     * "L16" encoding from RTP A/V Profile (RFC 3551).
     * Uncompressed samples coded as interleaved 16-bit signed big-endian
     * integers in two's complement notation.
     */
    ROC_PACKET_ENCODING_AVP_L16 = 2
} roc_packet_encoding;

/** Frame encoding. */
typedef enum roc_frame_encoding {
    /** PCM floats.
     * Uncompressed samples coded as floats in range [-1; 1].
     * Channels are interleaved, e.g. two channels are encoded as "L R L R ...".
     */
    ROC_FRAME_ENCODING_PCM_FLOAT = 1
} roc_frame_encoding;

/** Channel set. */
typedef enum roc_channel_set {
    /** Stereo.
     * Two channels: left and right.
     */
    ROC_CHANNEL_SET_STEREO = 2
} roc_channel_set;

/** Resampler profile. */
typedef enum roc_resampler_profile {
    /** No resampling. */
    ROC_RESAMPLER_DISABLE = -1,

    /** Default profile.
     * Current default is @c ROC_RESAMPLER_MEDIUM.
     */
    ROC_RESAMPLER_DEFAULT = 0,

    /** High quality, low speed. */
    ROC_RESAMPLER_HIGH = 1,

    /** Medium quality, medium speed. */
    ROC_RESAMPLER_MEDIUM = 2,

    /** Low quality, high speed. */
    ROC_RESAMPLER_LOW = 3
} roc_resampler_profile;

/** Clock source for sender or receiver. */
typedef enum roc_clock_source {
    /** Sender or receiver is clocked by external user-defined clock.
     * Write and read operations are non-blocking. The user is responsible
     * to call them in time, according to the external clock.
     */
    ROC_CLOCK_EXTERNAL = 0,

    /** Sender or receiver is clocked by an internal clock.
     * Write and read operations are blocking. They automatically wait until it's time
     * to process the next bunch of samples according to the configured sample rate.
     */
    ROC_CLOCK_INTERNAL = 1
} roc_clock_source;

/** Context configuration.
 * @see roc_context
 */
typedef struct roc_context_config {
    /** Maximum size in bytes of a network packet.
     * Defines the amount of bytes allocated per network packet.
     * Sender and receiver won't handle packets larger than this.
     * If zero, default value is used.
     */
    unsigned int max_packet_size;

    /** Maximum size in bytes of an audio frame.
     * Defines the amount of bytes allocated per intermediate internal frame in the
     * pipeline. Does not limit the size of the frames provided by user.
     * If zero, default value is used.
     */
    unsigned int max_frame_size;
} roc_context_config;

/** Sender configuration.
 * @see roc_sender
 */
typedef struct roc_sender_config {
    /** The rate of the samples in the frames passed to sender.
     * Number of samples per channel per second.
     * If @c frame_sample_rate and @c packet_sample_rate are different,
     * resampler should be enabled.
     * Should be set.
     */
    unsigned int frame_sample_rate;

    /** The channel set in the frames passed to sender.
     * Should be set.
     */
    roc_channel_set frame_channels;

    /** The sample encoding in the frames passed to sender.
     * Should be set.
     */
    roc_frame_encoding frame_encoding;

    /** The rate of the samples in the packets generated by sender.
     * Number of samples per channel per second.
     * If zero, default value is used.
     */
    unsigned int packet_sample_rate;

    /** The channel set in the packets generated by sender.
     * If zero, default value is used.
     */
    roc_channel_set packet_channels;

    /** The sample encoding in the packets generated by sender.
     * If zero, default value is used.
     */
    roc_packet_encoding packet_encoding;

    /** The length of the packets produced by sender, in nanoseconds.
     * Number of nanoseconds encoded per packet.
     * The samples written to the sender are buffered until the full packet is
     * accumulated or the sender is flushed or closed. Larger number reduces
     * packet overhead but also increases latency.
     * If zero, default value is used.
     */
    unsigned long long packet_length;

    /** Enable packet interleaving.
     * If non-zero, the sender shuffles packets before sending them. This
     * may increase robustness but also increases latency.
     */
    unsigned int packet_interleaving;

    /** Clock source to use.
     * Defines whether write operation will be blocking or non-blocking.
     * If zero, default value is used.
     */
    roc_clock_source clock_source;

    /** Resampler profile to use.
     * If non-zero, the sender employs resampler if the frame sample rate differs
     * from the packet sample rate.
     */
    roc_resampler_profile resampler_profile;

    /** FEC code to use.
     * If non-zero, the sender employs a FEC codec to generate redundant packets
     * which may be used on receiver to restore lost packets. This requires both
     * sender and receiver to use two separate source and repair ports.
     */
    roc_fec_code fec_code;

    /** Number of source packets per FEC block.
     * Used if some FEC code is selected.
     * Larger number increases robustness but also increases latency.
     * If zero, default value is used.
     */
    unsigned int fec_block_source_packets;

    /** Number of repair packets per FEC block.
     * Used if some FEC code is selected.
     * Larger number increases robustness but also increases traffic.
     * If zero, default value is used.
     */
    unsigned int fec_block_repair_packets;
} roc_sender_config;

/** Receiver configuration.
 * @see roc_receiver
 */
typedef struct roc_receiver_config {
    /** The rate of the samples in the frames returned to the user.
     * Number of samples per channel per second.
     * Should be set.
     */
    unsigned int frame_sample_rate;

    /** The channel set in the frames returned to the user.
     * Should be set.
     */
    roc_channel_set frame_channels;

    /** The sample encoding in the frames returned to the user.
     * Should be set.
     */
    roc_frame_encoding frame_encoding;

    /** Clock source to use.
     * Defines whether read operation will be blocking or non-blocking.
     * If zero, default value is used.
     */
    roc_clock_source clock_source;

    /** Resampler profile to use.
     * If non-zero, the receiver employs resampler for two purposes:
     *  - adjust the sender clock to the receiver clock, which may differ a bit
     *  - convert the packet sample rate to the frame sample rate if they are different
     */
    roc_resampler_profile resampler_profile;

    /** Target latency, in nanoseconds.
     * The session will not start playing until it accumulates the requested latency.
     * Then, if resampler is enabled, the session will adjust its clock to keep actual
     * latency as close as close as possible to the target latency.
     * If zero, default value is used.
     */
    unsigned long long target_latency;

    /** Maximum delta between current and target latency, in nanoseconds.
     * If current latency becomes larger than the target latency plus this value, the
     * session is terminated.
     * If zero, default value is used.
     */
    unsigned long long max_latency_overrun;

    /** Maximum delta between target and current latency, in nanoseconds.
     * If current latency becomes smaller than the target latency minus this value, the
     * session is terminated.
     * May be larger than the target latency because current latency may be negative,
     * which means that the playback run ahead of the last packet received from network.
     * If zero, default value is used.
     */
    unsigned long long max_latency_underrun;

    /** Timeout for the lack of playback, in nanoseconds.
     * If there is no playback during this period, the session is terminated.
     * This mechanism allows to detect dead, hanging, or broken clients
     * generating invalid packets.
     * If zero, default value is used. If negative, the timeout is disabled.
     */
    long long no_playback_timeout;

    /** Timeout for broken playback, in nanoseconds.
     * If there the playback is considered broken during this period, the session
     * is terminated. The playback is broken if there is a breakage detected at every
     * @c breakage_detection_window during @c broken_playback_timeout.
     * This mechanism allows to detect vicious circles like when all client packets
     * are a bit late and receiver constantly drops them producing unpleasant noise.
     * If zero, default value is used. If negative, the timeout is disabled.
     */
    long long broken_playback_timeout;

    /** Breakage detection window, in nanoseconds.
     * If zero, default value is used.
     * @see broken_playback_timeout.
     */
    unsigned long long breakage_detection_window;
} roc_receiver_config;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_CONFIG_H_
