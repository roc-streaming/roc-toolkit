/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/config.h
 * \brief Constants and configs.
 */

#ifndef ROC_CONFIG_H_
#define ROC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Network slot.
 *
 * A peer (sender or receiver) may have multiple slots, which may be independently
 * bound or connected. You can use multiple slots on sender to connect it to multiple
 * receiver addresses, and you can use multiple slots on receiver to bind it to
 * multiple receiver address.
 *
 * Slots are numbered from zero and are created implicitly. Just specify slot index
 * when binding or connecting endpoint, and slot will be automatically created if it
 * was not created yet.
 *
 * In simple cases, just use \c ROC_SLOT_DEFAULT.
 *
 * Inside each slot, there can be up to one endpoint for each interface type.
 * See \ref roc_interface for details.
 */
typedef unsigned int roc_slot;

/** Alias for the slot with index zero.
 * \see roc_slot
 */
static const roc_slot ROC_SLOT_DEFAULT = 0;

/** Network interface.
 *
 * Interface is a way to access the peer (sender or receiver) via network.
 *
 * Each peer slot has multiple interfaces, one of each type. The user interconnects
 * peers by binding one of the first peer's interfaces to an URI and then connecting the
 * corresponding second peer's interface to that URI.
 *
 * A URI is represented by \ref roc_endpoint object.
 *
 * The interface defines the type of the communication with the remote peer and the
 * set of protocols (URI schemes) that can be used with this particular interface.
 *
 * \c ROC_INTERFACE_CONSOLIDATED is an interface for high-level protocols which
 * automatically manage all necessary communication: transport streams, control messages,
 * parameter negotiation, etc. When a consolidated connection is established, peers may
 * automatically setup lower-level interfaces like \c ROC_INTERFACE_AUDIO_SOURCE, \c
 * ROC_INTERFACE_AUDIO_REPAIR, and \c ROC_INTERFACE_AUDIO_CONTROL.
 *
 * \c ROC_INTERFACE_CONSOLIDATED is mutually exclusive with lower-level interfaces.
 * In most cases, the user needs only \c ROC_INTERFACE_CONSOLIDATED. However, the
 * lower-level interfaces may be useful if an external signaling mechanism is used or for
 * compatibility with third-party software.
 *
 * \c ROC_INTERFACE_AUDIO_SOURCE and \c ROC_INTERFACE_AUDIO_REPAIR are lower-level
 * unidirectional transport-only interfaces. The first is used to transmit audio stream,
 * and the second is used to transmit redundant repair stream, if FEC is enabled.
 *
 * \c ROC_INTERFACE_AUDIO_CONTROL is a lower-level interface for control streams.
 * If you use \c ROC_INTERFACE_AUDIO_SOURCE and \c ROC_INTERFACE_AUDIO_REPAIR, you
 * usually also need to use \c ROC_INTERFACE_AUDIO_CONTROL to enable carrying additional
 * non-transport information.
 */
typedef enum roc_interface {
    /** Interface that consolidates all types of streams (source, repair, control).
     *
     * Allowed operations:
     *  - bind    (sender, receiver)
     *  - connect (sender, receiver)
     *
     * Allowed protocols:
     *  - \ref ROC_PROTO_RTSP
     */
    ROC_INTERFACE_CONSOLIDATED = 1,

    /** Interface for audio stream source data.
     *
     * Allowed operations:
     *  - bind    (receiver)
     *  - connect (sender)
     *
     * Allowed protocols:
     *  - \ref ROC_PROTO_RTP
     *  - \ref ROC_PROTO_RTP_RS8M_SOURCE
     *  - \ref ROC_PROTO_RTP_LDPC_SOURCE
     */
    ROC_INTERFACE_AUDIO_SOURCE = 11,

    /** Interface for audio stream repair data.
     *
     * Allowed operations:
     *  - bind    (receiver)
     *  - connect (sender)
     *
     * Allowed protocols:
     *  - \ref ROC_PROTO_RS8M_REPAIR
     *  - \ref ROC_PROTO_LDPC_REPAIR
     */
    ROC_INTERFACE_AUDIO_REPAIR = 12,

    /** Interface for audio control messages.
     *
     * Allowed operations:
     *  - bind    (sender, receiver)
     *  - connect (sender, receiver)
     *
     * Allowed protocols:
     *  - \ref ROC_PROTO_RTCP
     */
    ROC_INTERFACE_AUDIO_CONTROL = 13
} roc_interface;

/** Network protocol.
 *
 * Defines URI scheme of \ref roc_endpoint.
 */
typedef enum roc_protocol {
    /** RTSP 1.0 (RFC 2326) or RTSP 2.0 (RFC 7826).
     *
     * Interfaces:
     *  - \ref ROC_INTERFACE_CONSOLIDATED
     *
     * Transports:
     *   - for signaling: TCP
     *   - for media: RTP and RTCP over UDP or TCP
     */
    ROC_PROTO_RTSP = 10,

    /** RTP over UDP (RFC 3550).
     *
     * Interfaces:
     *  - \ref ROC_INTERFACE_AUDIO_SOURCE
     *
     * Transports:
     *  - UDP
     *
     * Audio encodings:
     *   - \ref ROC_PACKET_ENCODING_AVP_L16_MONO
     *   - \ref ROC_PACKET_ENCODING_AVP_L16_STEREO
     *
     * FEC encodings:
     *   - none
     */
    ROC_PROTO_RTP = 20,

    /** RTP source packet (RFC 3550) + FECFRAME Reed-Solomon footer (RFC 6865) with m=8.
     *
     * Interfaces:
     *  - \ref ROC_INTERFACE_AUDIO_SOURCE
     *
     * Transports:
     *  - UDP
     *
     * Audio encodings:
     *  - similar to \ref ROC_PROTO_RTP
     *
     * FEC encodings:
     *  - \ref ROC_FEC_ENCODING_RS8M
     */
    ROC_PROTO_RTP_RS8M_SOURCE = 30,

    /** FEC repair packet + FECFRAME Reed-Solomon header (RFC 6865) with m=8.
     *
     * Interfaces:
     *  - \ref ROC_INTERFACE_AUDIO_REPAIR
     *
     * Transports:
     *  - UDP
     *
     * FEC encodings:
     *  - \ref ROC_FEC_ENCODING_RS8M
     */
    ROC_PROTO_RS8M_REPAIR = 31,

    /** RTP source packet (RFC 3550) + FECFRAME LDPC-Staircase footer (RFC 6816).
     *
     * Interfaces:
     *  - \ref ROC_INTERFACE_AUDIO_SOURCE
     *
     * Transports:
     *  - UDP
     *
     * Audio encodings:
     *  - similar to \ref ROC_PROTO_RTP
     *
     * FEC encodings:
     *  - \ref ROC_FEC_ENCODING_LDPC_STAIRCASE
     */
    ROC_PROTO_RTP_LDPC_SOURCE = 32,

    /** FEC repair packet + FECFRAME LDPC-Staircase header (RFC 6816).
     *
     * Interfaces:
     *  - \ref ROC_INTERFACE_AUDIO_REPAIR
     *
     * Transports:
     *  - UDP
     *
     * FEC encodings:
     *  - \ref ROC_FEC_ENCODING_LDPC_STAIRCASE
     */
    ROC_PROTO_LDPC_REPAIR = 33,

    /** RTCP over UDP (RFC 3550).
     *
     * Interfaces:
     *  - \ref ROC_INTERFACE_AUDIO_CONTROL
     *
     * Transports:
     *  - UDP
     */
    ROC_PROTO_RTCP = 70
} roc_protocol;

/** Packet encoding.
 * Each packet encoding defines sample format, channel layout, and rate.
 * Each packet encoding is caompatible with specific protocols.
 */
typedef enum roc_packet_encoding {
    /** PCM signed 16-bit, 1 channel, 44100 rate.
     *
     * Represents 1-channel L16 stereo encoding from RTP A/V Profile (RFC 3551).
     * Uses uncompressed samples coded as interleaved 16-bit signed big-endian
     * integers in two's complement notation.
     *
     * Supported by protocols:
     *  - \ref ROC_PROTO_RTP
     *  - \ref ROC_PROTO_RTP_RS8M_SOURCE
     *  - \ref ROC_PROTO_RTP_LDPC_SOURCE
     */
    ROC_PACKET_ENCODING_AVP_L16_MONO = 1,

    /** PCM signed 16-bit, 2 channels, 44100 rate.
     *
     * Represents 2-channel L16 stereo encoding from RTP A/V Profile (RFC 3551).
     * Uses uncompressed samples coded as interleaved 16-bit signed big-endian
     * integers in two's complement notation.
     *
     * Supported by protocols:
     *  - \ref ROC_PROTO_RTP
     *  - \ref ROC_PROTO_RTP_RS8M_SOURCE
     *  - \ref ROC_PROTO_RTP_LDPC_SOURCE
     */
    ROC_PACKET_ENCODING_AVP_L16_STEREO = 2
} roc_packet_encoding;

/** Forward Error Correction encoding.
 * Each FEC encoding is caompatible with specific protocols.
 */
typedef enum roc_fec_encoding {
    /** No FEC encoding.
     * Compatible with \ref ROC_PROTO_RTP protocol.
     */
    ROC_FEC_ENCODING_DISABLE = -1,

    /** Default FEC encoding.
     * Current default is \ref ROC_FEC_ENCODING_RS8M.
     */
    ROC_FEC_ENCODING_DEFAULT = 0,

    /** Reed-Solomon FEC encoding (RFC 6865) with m=8.
     * Good for small block sizes (below 256 packets).
     * Compatible with \ref ROC_PROTO_RTP_RS8M_SOURCE and \ref ROC_PROTO_RS8M_REPAIR
     * protocols for source and repair endpoints.
     */
    ROC_FEC_ENCODING_RS8M = 1,

    /** LDPC-Staircase FEC encoding (RFC 6816).
     * Good for large block sizes (above 1024 packets).
     * Compatible with \ref ROC_PROTO_RTP_LDPC_SOURCE and \ref ROC_PROTO_LDPC_REPAIR
     * protocols for source and repair endpoints.
     */
    ROC_FEC_ENCODING_LDPC_STAIRCASE = 2
} roc_fec_encoding;

/** Sample format.
 * Defines how each sample is represented.
 * Does not define channels layout and sample rate.
 */
typedef enum roc_format {
    /** PCM floats.
     * Uncompressed samples coded as 32-bit native-endian floats in range [-1; 1].
     * Channels are interleaved, e.g. two channels are encoded as "L R L R ...".
     */
    ROC_FORMAT_PCM_FLOAT32 = 1
} roc_format;

/** Channel layout.
 * Defines number of channels and meaning of each channel.
 */
typedef enum roc_channel_layout {
    /** Mono.
     * One channel.
     */
    ROC_CHANNEL_LAYOUT_MONO = 1,

    /** Stereo.
     * Two channels: left and right.
     */
    ROC_CHANNEL_LAYOUT_STEREO = 2
} roc_channel_layout;

/** Media encoding.
 * Defines format and parameters of samples encoded in frames or packets.
 */
typedef struct roc_media_encoding {
    /** Sample frequency.
     * Defines number of samples per channel per second (e.g. 44100).
     */
    unsigned int rate;

    /** Sample format.
     * Defines sample precision and encoding.
     */
    roc_format format;

    /** Channel layout.
     * Defines number of channels and meaning of each channel.
     */
    roc_channel_layout channels;

    /** Multi-track channel count.
     * If \c channels is \c ROC_CHANNEL_LAYOUT_MULTITRACK, defines
     * number of channels (which represent independent "tracks").
     * For other channel layouts should be zero.
     */
    unsigned int tracks;
} roc_media_encoding;

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

/** Resampler backend.
 * Affects speed and quality.
 * Some backends may be disabled at build time.
 */
typedef enum roc_resampler_backend {
    /** Default backend.
     * Depends on what was enabled at build time.
     */
    ROC_RESAMPLER_BACKEND_DEFAULT = 0,

    /** Slow built-in resampler.
     * Always available.
     */
    ROC_RESAMPLER_BACKEND_BUILTIN = 1,

    /** Fast good-quality resampler from SpeexDSP.
     * May be disabled at build time.
     */
    ROC_RESAMPLER_BACKEND_SPEEX = 2
} roc_resampler_backend;

/** Resampler profile.
 * Affects speed and quality.
 * Each resampler backend treats profile in its own way.
 */
typedef enum roc_resampler_profile {
    /** Do not perform resampling.
     * Clock drift compensation will be disabled in this case.
     * If in doubt, do not disable resampling.
     */
    ROC_RESAMPLER_PROFILE_DISABLE = -1,

    /** Default profile.
     * Current default is \c ROC_RESAMPLER_PROFILE_MEDIUM.
     */
    ROC_RESAMPLER_PROFILE_DEFAULT = 0,

    /** High quality, low speed. */
    ROC_RESAMPLER_PROFILE_HIGH = 1,

    /** Medium quality, medium speed. */
    ROC_RESAMPLER_PROFILE_MEDIUM = 2,

    /** Low quality, high speed. */
    ROC_RESAMPLER_PROFILE_LOW = 3
} roc_resampler_profile;

/** Context configuration.
 *
 * It is safe to memset() this struct with zeros to get a default config. It is also
 * safe to memcpy() this struct to get a copy of config.
 *
 * \see roc_context
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
 *
 * For most fields, zero value means "use default", and you can memset() this struct
 * with zeros and then set only a few fields that don't have defaults. It is safe to
 * use memcpy() to get a copy of config, the struct is flat.
 *
 * \see roc_sender
 */
typedef struct roc_sender_config {
    /** The encoding used in frames passed to sender.
     * Frame encoding defines sample format, channel layout, and sample rate in local
     * frames created by user and passed to sender.
     * Should be set (zero value is invalid).
     */
    roc_media_encoding frame_encoding;

    /** The encoding used for packets produced by sender.
     * Packet encoding defines sample format, channel layout, and sample rate in network
     * packets. If packet encoding differs from frame encoding, conversion is performed
     * automatically. If sample rates are different, resampling should be enabled
     * via \c resampler_profile.
     * If zero, sender selects packet encoding automatically based on \c frame_encoding.
     * This automatic selection matches only encodings that have exact same sample rate
     * and channel layout, and hence don't require conversions. If you need conversions,
     * you should set packet encoding explicitly.
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

    /** FEC encoding to use.
     * If non-zero, the sender employs a FEC encoding to generate redundant packets
     * which may be used on receiver to restore lost packets. This requires both
     * sender and receiver to use two separate source and repair endpoints.
     */
    roc_fec_encoding fec_encoding;

    /** Number of source packets per FEC block.
     * Used if some FEC encoding is selected.
     * Larger number increases robustness but also increases latency.
     * If zero, default value is used.
     */
    unsigned int fec_block_source_packets;

    /** Number of repair packets per FEC block.
     * Used if some FEC encoding is selected.
     * Larger number increases robustness but also increases traffic.
     * If zero, default value is used.
     */
    unsigned int fec_block_repair_packets;

    /** Clock source to use.
     * Defines whether write operation will be blocking or non-blocking.
     * If zero, default value is used (\c ROC_CLOCK_EXTERNAL).
     */
    roc_clock_source clock_source;

    /** Resampler backend to use.
     * If zero, default value is used.
     */
    roc_resampler_backend resampler_backend;

    /** Resampler profile to use.
     * If non-zero, the sender employs resampler if the frame sample rate differs
     * from the packet sample rate.
     */
    roc_resampler_profile resampler_profile;
} roc_sender_config;

/** Receiver configuration.
 *
 * For most fields, zero value means "use default", and you can memset() this struct
 * with zeros and then set only a few fields that don't have defaults. It is safe to
 * use memcpy() to get a copy of config, the struct is flat.
 *
 * \see roc_receiver
 */
typedef struct roc_receiver_config {
    /** The encoding used in frames returned by receiver.
     * Frame encoding defines sample format, channel layout, and sample rate in local
     * frames returned by receiver to user.
     * Should be set (zero value is invalid).
     */
    roc_media_encoding frame_encoding;

    /** Clock source to use.
     * Defines whether read operation will be blocking or non-blocking.
     * If zero, default value is used (\c ROC_CLOCK_EXTERNAL).
     */
    roc_clock_source clock_source;

    /** Resampler backend to use.
     * If zero, default value is used.
     */
    roc_resampler_backend resampler_backend;

    /** Resampler profile to use.
     * If non-zero, the receiver employs resampler for two purposes:
     *  - adjust the sender clock to the receiver clock, to compensate clock drift
     *  - convert the packet sample rate to the frame sample rate if they are different
     */
    roc_resampler_profile resampler_profile;

    /** Target latency, in nanoseconds.
     * The session will not start playing until it accumulates the requested latency.
     * Then, if resampler is enabled, the session will adjust its clock to keep actual
     * latency as close as possible to the target latency.
     * If zero, default value is used.
     */
    unsigned long long target_latency;

    /** Maximum allowed delta between current and target latency, in nanoseconds.
     * If session latency differs from the target latency by more than given value, the
     * session is terminated (it can then automatically restart). Receiver itself is
     * not terminated; if there are no sessions, it will produce zeros.
     * If zero, default value is used.
     */
    unsigned long long latency_tolerance;

    /** Timeout for the lack of playback, in nanoseconds.
     * If there is no playback during this period, the session is terminated (it can
     * then automatically restart). Receiver itself is not terminated; if there are
     * no sessions, it will produce zeros.
     * This mechanism allows to detect dead, hanging, or incompatible clients that
     * generate unparseable packets.
     * If zero, default value is used. If negative, the timeout is disabled.
     */
    long long no_playback_timeout;

    /** Timeout for choppy playback, in nanoseconds.
     * If there is constant shuttering during this period, the session is terminated (it
     * can then automatically restart). Receiver itself is  not terminated; if there are
     * no sessions, it will produce zeros.
     * This mechanism allows to detect situations when playback continues but there
     * are frequent glitches, for example because there is a high ratio of late packets.
     * If zero, default value is used. If negative, the timeout is disabled.
     */
    long long choppy_playback_timeout;
} roc_receiver_config;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_CONFIG_H_
