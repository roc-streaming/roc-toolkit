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

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Network slot.
 *
 * A peer (sender or receiver) may have multiple slots, which may be independently
 * bound or connected. You can use multiple slots on sender to connect it to multiple
 * receiver addresses, or you can use multiple slots on receiver to bind it to
 * multiple receiver addresses.
 *
 * Inside each slot, there can be up to one endpoint for each interface type, for
 * example one source endpoint, one control endpoint, and so on. See \ref roc_interface
 * for more details.
 *
 * Slots are created implicitly. Just specify slot index when binding or connecting
 * endpoint, and slot will be automatically created if it doesn't exist yet.
 * Slot indices can be arbitrary numbers and does not need to be continuous.
 *
 * In simple cases, when one slot is enough, just use \c ROC_SLOT_DEFAULT,
 * which is an alias for slot index zero.
 */
typedef unsigned long long roc_slot;

/** Alias for the slot with index zero.
 * Has no special meaning. Exists for convenience, since most times
 * user doesn't need to bother about multiple slots.
 * \see roc_slot
 */
static const roc_slot ROC_SLOT_DEFAULT = 0;

/** Network interface.
 *
 * Interface is a way to access the peer (sender or receiver) via network.
 *
 * Each slot of a peer (see \ref roc_slot) has multiple interfaces, one for each of the
 * interface types. The user interconnects peers by binding an interface of one peer to
 * an URI, and then connecting the corresponding interface of another peer to that URI.
 *
 * A URI is represented by \ref roc_endpoint object.
 *
 * The interface defines the type of the communication with the remote peer and the
 * set of protocols (URI schemes) that can be used with this particular interface.
 *
 * \c ROC_INTERFACE_AGGREGATE is an interface for high-level protocols which
 * automatically manage all necessary communication: transport streams, control messages,
 * parameter negotiation, etc. When an aggregate connection is established, peers may
 * automatically setup lower-level interfaces like \c ROC_INTERFACE_AUDIO_SOURCE, \c
 * ROC_INTERFACE_AUDIO_REPAIR, and \c ROC_INTERFACE_AUDIO_CONTROL.
 *
 * \c ROC_INTERFACE_AGGREGATE is mutually exclusive with lower-level interfaces.
 * In most cases, the user needs only \c ROC_INTERFACE_AGGREGATE. However, the
 * lower-level interfaces may be useful if an external signaling mechanism is used or for
 * compatibility with third-party software.
 *
 * \c ROC_INTERFACE_AUDIO_SOURCE and \c ROC_INTERFACE_AUDIO_REPAIR are lower-level
 * unidirectional transport-only interfaces. The first is used to transmit audio stream,
 * and the second is used to transmit redundant repair stream, if FEC is enabled.
 *
 * \c ROC_INTERFACE_AUDIO_CONTROL is a lower-level bidirectional interface for control
 * streams. If you use \c ROC_INTERFACE_AUDIO_SOURCE and \c ROC_INTERFACE_AUDIO_REPAIR,
 * you usually also need to use \c ROC_INTERFACE_AUDIO_CONTROL to enable carrying
 * additional non-transport information.
 */
typedef enum roc_interface {
    /** Interface that aggregates multiple types of streams (source, repair, control).
     *
     * Allowed operations:
     *  - bind    (sender, receiver)
     *  - connect (sender, receiver)
     *
     * Allowed protocols:
     *  - \ref ROC_PROTO_RTSP
     */
    ROC_INTERFACE_AGGREGATE = 1,

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
     *  - \ref ROC_INTERFACE_AGGREGATE
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
     *   - encodings registered using roc_context_register_encoding()
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
 * Each packet encoding is compatible with specific protocols.
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
    ROC_PACKET_ENCODING_AVP_L16_MONO = 11,

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
    ROC_PACKET_ENCODING_AVP_L16_STEREO = 10,
} roc_packet_encoding;

/** Forward Error Correction encoding.
 * Each FEC encoding is compatible with specific protocols.
 */
typedef enum roc_fec_encoding {
    /** No FEC encoding.
     *
     * Compatible with \ref ROC_PROTO_RTP protocol.
     *
     * Pros:
     *  - compatible with third-party software that does not support FECFRAME
     *
     * Cons:
     *  - no packet recovery
     */
    ROC_FEC_ENCODING_DISABLE = -1,

    /** Default FEC encoding.
     * Current default is \ref ROC_FEC_ENCODING_RS8M.
     */
    ROC_FEC_ENCODING_DEFAULT = 0,

    /** Reed-Solomon FEC encoding (RFC 6865) with m=8.
     *
     * Good for small block sizes (below 256 packets).
     * Compatible with \ref ROC_PROTO_RTP_RS8M_SOURCE and \ref ROC_PROTO_RS8M_REPAIR
     * protocols for source and repair endpoints.
     *
     * Pros:
     *  - good repair capabilities even on small block sizes
     *
     * Cons:
     *  - high CPU usage on large block sizes
     */
    ROC_FEC_ENCODING_RS8M = 1,

    /** LDPC-Staircase FEC encoding (RFC 6816).
     *
     * Good for large block sizes (above 1024 packets).
     * Compatible with \ref ROC_PROTO_RTP_LDPC_SOURCE and \ref ROC_PROTO_LDPC_REPAIR
     * protocols for source and repair endpoints.
     *
     * Pros:
     *  - low CPU usage even on large block sizes
     *
     * Cons:
     *  - low repair capabilities on small block sizes
     */
    ROC_FEC_ENCODING_LDPC_STAIRCASE = 2
} roc_fec_encoding;

/** Sample format.
 *
 * Format (\c roc_format) and sub-format (\c roc_subformat) together define how samples
 * are encoded into binary form (typically local frames or network packets). Each format
 * allows use of certain sub-formats.
 *
 * Format and sub-format don't define sample rate and channels layout, these parameters
 * are configured separately via \ref roc_media_encoding.
 */
typedef enum roc_format {
    /** Uncompressed interleaved headerless PCM samples.
     *
     * Multiple channels are interleaved, e.g. two channels are encoded as "L R L R ...".
     *
     * - supported subformats: \c ROC_SUBFORMAT_PCM_*
     * - supported rates: any
     * - supported channels: any
     */
    ROC_FORMAT_PCM = 1,
} roc_format;

/** Sample sub-format.
 * Defines samples representation together with \c roc_format.
 */
typedef enum roc_subformat {
    /** 8-bit signed integer. */
    ROC_SUBFORMAT_PCM_SINT8 = 40,
    /** 8-bit unsigned integer. */
    ROC_SUBFORMAT_PCM_UINT8 = 41,

    /** 16-bit signed integer, native endian. */
    ROC_SUBFORMAT_PCM_SINT16 = 50,
    /** 16-bit signed integer, little endian. */
    ROC_SUBFORMAT_PCM_SINT16_LE = 51,
    /** 16-bit signed integer, big endian. */
    ROC_SUBFORMAT_PCM_SINT16_BE = 52,
    /** 16-bit unsigned integer, native endian. */
    ROC_SUBFORMAT_PCM_UINT16 = 53,
    /** 16-bit unsigned integer, little endian. */
    ROC_SUBFORMAT_PCM_UINT16_LE = 54,
    /** 16-bit unsigned integer, big endian. */
    ROC_SUBFORMAT_PCM_UINT16_BE = 55,

    /** 24-bit signed integer, native endian. */
    ROC_SUBFORMAT_PCM_SINT24 = 60,
    /** 24-bit signed integer, little endian. */
    ROC_SUBFORMAT_PCM_SINT24_LE = 61,
    /** 24-bit signed integer, big endian. */
    ROC_SUBFORMAT_PCM_SINT24_BE = 62,
    /** 24-bit unsigned integer, native endian. */
    ROC_SUBFORMAT_PCM_UINT24 = 63,
    /** 24-bit unsigned integer, little endian. */
    ROC_SUBFORMAT_PCM_UINT24_LE = 64,
    /** 24-bit unsigned integer, big endian. */
    ROC_SUBFORMAT_PCM_UINT24_BE = 65,

    /** 32-bit signed integer, native endian. */
    ROC_SUBFORMAT_PCM_SINT32 = 70,
    /** 32-bit signed integer, little endian. */
    ROC_SUBFORMAT_PCM_SINT32_LE = 71,
    /** 32-bit signed integer, big endian. */
    ROC_SUBFORMAT_PCM_SINT32_BE = 72,
    /** 32-bit unsigned integer, native endian. */
    ROC_SUBFORMAT_PCM_UINT32 = 73,
    /** 32-bit unsigned integer, little endian. */
    ROC_SUBFORMAT_PCM_UINT32_LE = 74,
    /** 32-bit unsigned integer, big endian. */
    ROC_SUBFORMAT_PCM_UINT32_BE = 75,

    /** 64-bit signed integer, native endian. */
    ROC_SUBFORMAT_PCM_SINT64 = 80,
    /** 64-bit signed integer, little endian. */
    ROC_SUBFORMAT_PCM_SINT64_LE = 81,
    /** 64-bit signed integer, big endian. */
    ROC_SUBFORMAT_PCM_SINT64_BE = 82,
    /** 64-bit unsigned integer, native endian. */
    ROC_SUBFORMAT_PCM_UINT64 = 83,
    /** 64-bit unsigned integer, little endian. */
    ROC_SUBFORMAT_PCM_UINT64_LE = 84,
    /** 64-bit unsigned integer, big endian. */
    ROC_SUBFORMAT_PCM_UINT64_BE = 85,

    /** 32-bit IEEE-754 float in range [-1.0; +1.0], native endian. */
    ROC_SUBFORMAT_PCM_FLOAT32 = 90,
    /** 32-bit IEEE-754 float in range [-1.0; +1.0], little endian. */
    ROC_SUBFORMAT_PCM_FLOAT32_LE = 91,
    /** 32-bit IEEE-754 float in range [-1.0; +1.0], big endian. */
    ROC_SUBFORMAT_PCM_FLOAT32_BE = 92,
    /** 64-bit IEEE-754 float in range [-1.0; +1.0], native endian. */
    ROC_SUBFORMAT_PCM_FLOAT64 = 93,
    /** 64-bit IEEE-754 float in range [-1.0; +1.0], little endian. */
    ROC_SUBFORMAT_PCM_FLOAT64_LE = 94,
    /** 64-bit IEEE-754 float in range [-1.0; +1.0], big endian. */
    ROC_SUBFORMAT_PCM_FLOAT64_BE = 95,
} roc_subformat;

/** Channel layout.
 * Defines number of channels and meaning of each channel.
 */
typedef enum roc_channel_layout {
    /** Mono.
     * One channel with monophonic sound.
     */
    ROC_CHANNEL_LAYOUT_MONO = 1,

    /** Stereo.
     * Two channels: left, right.
     */
    ROC_CHANNEL_LAYOUT_STEREO = 2,

    /** Multi-track audio.
     *
     * In multitrack layout, stream contains multiple channels which represent
     * independent "tracks" without any special meaning (unlike stereo or surround)
     * and hence without any special processing or mapping.
     *
     * The number of channels is arbitrary and is defined by \c tracks field of
     * \ref roc_media_encoding struct.
     */
    ROC_CHANNEL_LAYOUT_MULTITRACK = 4
} roc_channel_layout;

/** Media encoding.
 * Defines format and parameters of samples encoded in frames or packets.
 */
typedef struct roc_media_encoding {
    /** Sample format.
     * Defines sample binary coding.
     */
    roc_format format;

    /** Sample sub-format.
     * Defines sample binary representation together with \c format.
     * Allowed values depend on \c format.
     */
    roc_subformat subformat;

    /** Sample frequency.
     * Defines number of samples per channel per second (e.g. 44100).
     * Allowed values may be limited by \c format.
     */
    unsigned int rate;

    /** Channel layout.
     * Defines number of channels and meaning of each channel.
     * Allowed values may be limited by \c format.
     */
    roc_channel_layout channels;

    /** Multi-track channel count.
     *
     * If \c channels is \ref ROC_CHANNEL_LAYOUT_MULTITRACK, defines
     * number of channels (which represent independent "tracks").
     * For other channel layouts should be zero.
     *
     * Should be in range [1; 1024].
     */
    unsigned int tracks;
} roc_media_encoding;

/** Clock source for sender or receiver.
 * Defines who is responsible to invoke read or write in proper time.
 */
typedef enum roc_clock_source {
    /** Default clock source.
     * Current default is \c ROC_CLOCK_SOURCE_EXTERNAL.
     */
    ROC_CLOCK_SOURCE_DEFAULT = 0,

    /** Sender or receiver is clocked by external user-defined clock.
     *
     * Write and read operations are non-blocking. The user is responsible
     * to call them in time, according to the external clock.
     *
     * Use when samples source (from where you read them to pass to receiver)
     * or destination (to where you write them after obtaining from sender)
     * is active and has its own clock, e.g. it is a sound card.
     */
    ROC_CLOCK_SOURCE_EXTERNAL = 1,

    /** Sender or receiver is clocked by an internal pipeline clock.
     *
     * Write and read operations are blocking. They automatically wait until it's time
     * to process the next bunch of samples according to the configured sample rate,
     * based on a CPU timer.
     *
     * Use when samples source (from where you read them to pass to receiver)
     * or destination (to where you write them after obtaining from sender)
     * is passive and does now have clock, e.g. it is a file on disk.
     */
    ROC_CLOCK_SOURCE_INTERNAL = 2
} roc_clock_source;

/** Latency tuner backend.
 * Defines which latency is monitored and adjusted by latency tuner.
 */
typedef enum roc_latency_tuner_backend {
    /** Default backend.
     * Current default is \c ROC_LATENCY_TUNER_BACKEND_NIQ.
     */
    ROC_LATENCY_TUNER_BACKEND_DEFAULT = 0,

    /** Latency tuning is based on network incoming queue length.
     *
     * In this mode, latency is defined as incoming queue length (in nanoseconds).
     * Latency tuner monitors queue length and and adjusts playback clock speed
     * to keep queue length close to configured target latency.
     *
     * Keeping constant queue length allows to match playback clock speed with the
     * capture clock speed and to keep the overall latency constant (yet unknown).
     *
     * On receiver, this backend is always available, without any protocol help.
     * On sender, this backend works only if RTCP is enabled and both sender and
     * receiver are implemented with roc-toolkit, as it relies on a non-standard
     * RTCP extension to report receiver queue length to sender.
     *
     * Pros:
     *  - works with any protocol if used on receiver (does not require RTCP or NTP)
     *
     * Cons:
     *  - synchronizes only clock speed, but not position; different receivers will
     *    have different (constant, on average) delays
     *  - affected by network jitter; spikes in packet delivery will cause slow
     *    oscillations in clock speed
     */
    ROC_LATENCY_TUNER_BACKEND_NIQ = 2
} roc_latency_tuner_backend;

/** Latency tuner profile.
 * Defines whether latency adjustment is enabled and which algorithm is used.
 */
typedef enum roc_latency_tuner_profile {
    /** Default profile.
     *
     * On receiver, when \c ROC_LATENCY_TUNER_BACKEND_NIQ is used, selects
     * \c ROC_LATENCY_TUNER_PROFILE_RESPONSIVE if target latency is low, and
     * \c ROC_LATENCY_TUNER_PROFILE_GRADUAL if target latency is high.
     *
     * On sender, selects \c ROC_LATENCY_TUNER_PROFILE_INTACT.
     */
    ROC_LATENCY_TUNER_PROFILE_DEFAULT = 0,

    /** No latency adjustment.
     *
     * In this mode, clock speed is not adjusted.
     *
     * You can set this mode on receiver, and set some other mode on sender, to
     * do latency adjustment on sender side instead of receiver side. It's useful
     * when receiver is CPU-constrained and sender is not, because latency tuner
     * relies on resampling, which is increases CPU usage.
     *
     * You can also set this mode on both sender and receiver if you don't need
     * latency adjustment at all. However, if sender and receiver have independent
     * clocks (which is typically the case), clock drift will lead to periodic
     * playback disruptions caused by underruns and overruns.
     */
    ROC_LATENCY_TUNER_PROFILE_INTACT = 1,

    /** Responsive latency adjustment.
     *
     * Clock speed is adjusted quickly and accurately.
     *
     * Requires high precision clock adjustment, hence recommended for use with
     * \ref ROC_RESAMPLER_BACKEND_BUILTIN.
     *
     * Pros:
     *  - allows very low latency and synchronization error
     *
     * Cons:
     *  - does not work well with some resampler backends
     *  - does not work well with \ref ROC_LATENCY_TUNER_BACKEND_NIQ
     *    if network jitter is high
     */
    ROC_LATENCY_TUNER_PROFILE_RESPONSIVE = 2,

    /** Gradual latency adjustment.
     *
     * Clock speed is adjusted slowly and smoothly.
     *
     * Pros:
     *  - works well even with high network jitter
     *  - works well with any resampler backend
     *
     * Cons:
     *  - does not allow very low latency and synchronization error
     */
    ROC_LATENCY_TUNER_PROFILE_GRADUAL = 3
} roc_latency_tuner_profile;

/** Resampler backend.
 * Affects CPU usage, quality, and clock synchronization precision.
 * Some backends may be disabled at build time.
 */
typedef enum roc_resampler_backend {
    /** Default backend.
     *
     * Selects \ref ROC_RESAMPLER_BACKEND_BUILTIN when using \ref
     * ROC_LATENCY_TUNER_PROFILE_RESPONSIVE, or when SpeexDSP is disabled.
     *
     * Otherwise, selects \ref ROC_RESAMPLER_BACKEND_SPEEX.
     */
    ROC_RESAMPLER_BACKEND_DEFAULT = 0,

    /** CPU-intensive good-quality high-precision built-in resampler.
     *
     * This backend controls clock speed with very high precision, and hence is useful
     * when latency or synchronization error should be very low.
     *
     * This backend has higher CPU usage, especially on high resampling quality and on
     * CPUs with small L3 caches.
     *
     * The implementation is based on bandlimited interpolation algorithm.
     *
     * This backend is always available.
     *
     * Recommended for \ref ROC_LATENCY_TUNER_PROFILE_RESPONSIVE and on good CPUs.
     */
    ROC_RESAMPLER_BACKEND_BUILTIN = 1,

    /** Fast good-quality low-precision resampler based on SpeexDSP.
     *
     * This backend has low CPU usage even on high resampler quality and cheap CPUs.
     *
     * This backend controls clock speed with lower precision, and is not so good when
     * latency or synchronization error should be very low.
     *
     * This backend is available only when SpeexDSP was enabled at build time.
     *
     * Recommended for \ref ROC_LATENCY_TUNER_PROFILE_GRADUAL and on cheap CPUs.
     */
    ROC_RESAMPLER_BACKEND_SPEEX = 2,

    /** Fast medium-quality and medium-precision resampler combining SpeexDSP with
     * decimation.
     *
     * This backend uses SpeexDSP for converting between base rates (e.g. 44100 vs 48000)
     * and decimation/expansion (dropping or duplicating samples) for clock drift
     * compensation.
     *
     * Typical decimation rate needed to compensate clock drift is below 0.5ms/second
     * (20 samples/second on 48Khz), which gives tolerable quality despite usage of
     * decimation, especially for speech.
     *
     * When frame and packet sample rates are equal (e.g. both are 44100), only decimation
     * stage is needed, and this becomes fastest possible backend working almost as fast
     * as memcpy().
     *
     * When frame and packet rates are different, usage of this backend, compared to
     * \c ROC_RESAMPLER_BACKEND_SPEEX, allows to sacrifice some quality and improve
     * scaling precision and CPU usage in return.
     *
     * This backend is available only when SpeexDSP was enabled at build time.
     *
     * Recommended when CPU resources are extremely limited.
     */
    ROC_RESAMPLER_BACKEND_SPEEXDEC = 3
} roc_resampler_backend;

/** Resampler profile.
 * Affects CPU usage and quality.
 * Each resampler backend treats profile in its own way.
 */
typedef enum roc_resampler_profile {
    /** Default profile.
     * Current default is \c ROC_RESAMPLER_PROFILE_MEDIUM.
     */
    ROC_RESAMPLER_PROFILE_DEFAULT = 0,

    /** High quality, higher CPU usage. */
    ROC_RESAMPLER_PROFILE_HIGH = 1,

    /** Medium quality, medium CPU usage. */
    ROC_RESAMPLER_PROFILE_MEDIUM = 2,

    /** Low quality, lower CPU usage. */
    ROC_RESAMPLER_PROFILE_LOW = 3
} roc_resampler_profile;

/** PLC backend.
 *
 * Packet loss concealment (PLC), is used to reduce distortion caused by lost
 * packets by filling gaps with interpolated or extrapolated data.
 *
 * PLC is used when a packet was lost and FEC was not able to recover it.
 */
typedef enum roc_plc_backend {
    /** No PLC.
     * Gaps are filled with zeros (silence).
     */
    ROC_PLC_BACKEND_DISABLE = -1,

    /** Default backend.
     * Current default is \c ROC_PLC_BACKEND_DISABLE.
     */
    ROC_PLC_BACKEND_DEFAULT = 0,
} roc_plc_backend;

/** Context configuration.
 *
 * It is safe to memset() this struct with zeros to get a default config. It is also
 * safe to memcpy() this struct to get a copy of config.
 *
 * \see roc_context
 */
typedef struct roc_context_config {
    /** Maximum size in bytes of a network packet.
     *
     * Defines the amount of bytes allocated per network packet.
     * Sender and receiver won't handle packets larger than this.
     *
     * If zero, default value is used.
     */
    unsigned int max_packet_size;

    /** Maximum size in bytes of an audio frame.
     *
     * Defines the amount of bytes allocated per intermediate internal frame in the
     * pipeline. Does not limit the size of the frames provided by user.
     *
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
     *
     * Frame encoding defines sample format, channel layout, and sample rate in **local**
     * frames created by user and passed to sender.
     *
     * Should be set explicitly (zero value is invalid).
     */
    roc_media_encoding frame_encoding;

    /** The encoding used for packets produced by sender.
     *
     * Packet encoding defines sample format, channel layout, and sample rate in network
     * packets. If packet encoding differs from frame encoding, conversion is performed
     * automatically.
     *
     * If zero, sender selects packet encoding automatically based on \c frame_encoding.
     * This automatic selection matches only encodings that have exact same sample rate,
     * channel layout, and format, hence don't require conversions. If you need
     * conversions, you should set packet encoding explicitly.
     *
     * You can use \ref roc_context_register_encoding() to register custom encoding, and
     * set \c packet_encoding to registered identifier. If you use signaling protocol like
     * RTSP, it's enough to register in just on sender; otherwise, you need to do the same
     * on receiver as well.
     */
    roc_packet_encoding packet_encoding;

    /** The length of the packets produced by sender, in nanoseconds.
     *
     * Number of nanoseconds encoded per packet.
     * The samples written to the sender are buffered until the full packet is
     * accumulated or the sender is flushed or closed. Larger number reduces
     * packet overhead but also does not allow smaller latency.
     *
     * If zero, default value is used.
     */
    unsigned long long packet_length;

    /** FEC encoding to use.
     *
     * If FEC is enabled, the sender employs a FEC encoding to generate redundant
     * packet which may be used on receiver to restore lost packets. This requires
     * both sender and receiver to use two separate source and repair endpoints.
     *
     * If zero, default encoding is used (\ref ROC_FEC_ENCODING_DEFAULT).
     */
    roc_fec_encoding fec_encoding;

    /** Number of source packets per FEC block.
     * Used if some FEC encoding is selected.
     *
     * Sender divides stream into blocks of N source (media) packets, and adds M repair
     * (redundancy) packets to each block, where N is \c fec_block_source_packets and M
     * is \c fec_block_repair_packets.
     *
     * Larger number of source packets in block increases robustness (repair ratio), but
     * also increases latency.
     *
     * If zero, default value is used.
     */
    unsigned int fec_block_source_packets;

    /** Number of repair packets per FEC block.
     * Used if some FEC encoding is selected.
     *
     * Sender divides stream into blocks of N source (media) packets, and adds M repair
     * (redundancy) packets to each block, where N is \c fec_block_source_packets and M
     * is \c fec_block_repair_packets.
     *
     * Larger number of repair packets in block increases robustness (repair ratio), but
     * also increases traffic. Number of repair packets usually should be 1/2 or 2/3 of
     * the number of source packets.
     *
     * If zero, default value is used.
     */
    unsigned int fec_block_repair_packets;

    /** Clock source to use.
     * Defines whether write operation is blocking or non-blocking.
     *
     * If write is non-blocking, the user is responsible to invoke it in appropriate
     * time. Otherwise it will automatically wait for that time.
     *
     * If zero, default value is used (\ref ROC_CLOCK_SOURCE_DEFAULT).
     */
    roc_clock_source clock_source;

    /** Latency tuner backend.
     * Defines which latency is monitored and controlled by latency tuner.
     * Defines semantics of \c target_latency and related fields.
     *
     * If zero, default backend is used (\ref ROC_LATENCY_TUNER_BACKEND_DEFAULT).
     */
    roc_latency_tuner_backend latency_tuner_backend;

    /** Latency tuner profile.
     * Defines whether latency adjustment is enabled and which algorithm is used.
     *
     * If zero, default profile is used (\ref ROC_LATENCY_TUNER_PROFILE_DEFAULT).
     *
     * By default, latency adjustment is **disabled** on sender (\c latency_tuner_profile
     * is \ref ROC_LATENCY_TUNER_PROFILE_INTACT). If you enable it on sender, you
     * need to disable it on receiver.
     */
    roc_latency_tuner_profile latency_tuner_profile;

    /** Resampler backend.
     * Affects CPU usage, quality, and clock synchronization precision
     * (if latency adjustment is enabled).
     *
     * If zero, default backend is used (\ref ROC_RESAMPLER_BACKEND_DEFAULT).
     */
    roc_resampler_backend resampler_backend;

    /** Resampler profile.
     * Affects CPU usage and quality.
     *
     * If zero, default profile is used (\ref ROC_RESAMPLER_PROFILE_DEFAULT).
     */
    roc_resampler_profile resampler_profile;

    /** Target latency, in nanoseconds.
     *
     * By default, latency adjustment is enabled on receiver and disabled on sender
     * (see \c latency_tuner_profile). In this case, \c target_latency,
     * \c latency_tolerance, \c start_target_latency, \c min_target_latency, and
     * \c max_target_latency should be configured only on receiver, and should be set
     * to zeros on sender.
     *
     * You can enable latency adjustment on sender and disable it on receiver
     * (again, see \c latency_tuner_profile). In this case, \c target_latency,
     * \c latency_tolerance, \c start_target_latency, \c min_target_latency, and
     * \c max_target_latency should be configured on both sender and receiver and
     * should match each other.
     *
     * Semantics of the fields on sender is the same as on receiver. Refer to comments
     * in \ref roc_receiver_config for details on each field.
     */
    unsigned long long target_latency;

    /** Maximum allowed delta between current and target latency, in nanoseconds.
     *
     * By default, latency adjustment is enabled on receiver and disabled on sender
     * (see \c latency_tuner_profile), and this field isn't used and should be zero.
     *
     * If you want to enable it, refer to the comment for \c target_latency.
     */
    unsigned long long latency_tolerance;

    /** Starting latency for adaptive mode, in nanoseconds.
     *
     * By default, latency adjustment is enabled on receiver and disabled on sender
     * (see \c latency_tuner_profile), and this field isn't used and should be zero.
     *
     * If you want to enable it, refer to the comment for \c target_latency.
     */
    unsigned long long start_target_latency;

    /** Minimum latency for adaptive mode, in nanoseconds.
     *
     * By default, latency adjustment is enabled on receiver and disabled on sender
     * (see \c latency_tuner_profile), and this field isn't used and should be zero.
     *
     * If you want to enable it, refer to the comment for \c target_latency.
     */
    unsigned long long min_target_latency;

    /** Maximum latency for adaptive mode, in nanoseconds.
     *
     * By default, latency adjustment is enabled on receiver and disabled on sender
     * (see \c latency_tuner_profile), and this field isn't used and should be zero.
     *
     * If you want to enable it, refer to the comment for \c target_latency.
     */
    unsigned long long max_target_latency;
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
     *
     * Frame encoding defines sample format, channel layout, and sample rate in **local**
     * frames returned by receiver to user.
     *
     * Should be set (zero value is invalid).
     */
    roc_media_encoding frame_encoding;

    /** Clock source.
     * Defines whether read operation is blocking or non-blocking.
     *
     * If read is non-blocking, the user is responsible to invoke it in appropriate
     * time. Otherwise it will automatically wait for that time.
     *
     * If zero, default value is used (\ref ROC_CLOCK_SOURCE_DEFAULT).
     */
    roc_clock_source clock_source;

    /** Latency tuner backend.
     * Defines which latency is monitored and controlled by latency tuner.
     * Defines semantics of \c target_latency and related fields.
     *
     * If zero, default backend is used (\ref ROC_LATENCY_TUNER_BACKEND_DEFAULT).
     */
    roc_latency_tuner_backend latency_tuner_backend;

    /** Latency tuner profile.
     * Defines whether latency adjustment is enabled and which algorithm is used.
     *
     * If zero, default profile is used (\ref ROC_LATENCY_TUNER_PROFILE_DEFAULT).
     *
     * By default, latency adjustment is **enabled** on receiver (\c latency_tuner_profile
     * is not \ref ROC_LATENCY_TUNER_PROFILE_INTACT). If you disable it on receiver,
     * you usually need to enable it on sender.
     */
    roc_latency_tuner_profile latency_tuner_profile;

    /** Resampler backend.
     * Affects CPU usage, quality, and clock synchronization precision
     * (if latency adjustment is enabled).
     *
     * If zero, default backend is used (\ref ROC_RESAMPLER_BACKEND_DEFAULT).
     */
    roc_resampler_backend resampler_backend;

    /** Resampler profile.
     * Affects CPU usage and quality.
     *
     * If zero, default profile is used (\ref ROC_RESAMPLER_PROFILE_DEFAULT).
     */
    roc_resampler_profile resampler_profile;

    /** PLC backend.
     * Allows to reduce distortion cased by packet loss.
     *
     * If zero, default backend is used (\ref ROC_PLC_BACKEND_DEFAULT).
     *
     * You can use \ref roc_context_register_plc() to register custom PLC implementation,
     * and set \c plc_backend to registered identifier.
     */
    roc_plc_backend plc_backend;

    /** Target latency, in nanoseconds.
     *
     * Defines the latency value to maintain, as measured by the latency backend
     * (see \c latency_tuner_backend):
     *   - Non-zero value activates **fixed latency** mode: the latency starts from
     *     \c target_latency and is kept close to that value.
     *   - Zero value activates **adaptive latency** mode: the latency is chosen
     *     dynamically. Initial latency is \c start_target_latency, and the allowed
     *     range is \c min_target_latency to \c max_target_latency.
     *     Latency tuner consistently reassesses network conditions and changes target
     *     to achieve the lowest latency that doesn't cause disruptions.
     *
     * If latency adjustment is enabled on receiver (default setting, see
     * \c latency_tuner_profile), receiver starts with the initial latency and
     * continuously modulates clock speed to keep actual latency close to the target.
     * It also validates that the latency deviation never exceeds the limit (see
     * \c latency_tolerance).
     *
     * If latency adjustment is disabled on receiver, it is should be enabled on sender.
     * In this case, receiver starts with the initial latency, but afterwards does not
     * try to adjust clock speed, assuming that sender will do it. However, receiver
     * still validates that the latency deviation doesn't exceed the limit.
     *
     * When latency adjustment is enabled on sender instead of receiver,
     * \c target_latency, \c start_target_latency, \c min_target_latency and
     * \c max_target_latency should match on sender and receiver.
     * This ensures both sides know the initial latency and the allowed range.
     */
    unsigned long long target_latency;

    /** Maximum allowed delta between current and target latency, in nanoseconds.
     *
     * Latency tuner continuously monitors deviation of actual latency from target.
     * If deviation becomes bigger than \c latency_tolerance, connection to sender
     * is terminated (but sender may reconnect).
     *
     * If zero, default value is used.
     */
    unsigned long long latency_tolerance;

    /** Starting latency for adaptive mode, in nanoseconds.
     *
     * If adaptive latency mode is used (default setting, see \c target_latency), this
     * field defines initial value for the target latency.
     *
     * If zero, default value is used.
     */
    unsigned long long start_target_latency;

    /** Minimum latency for adaptive mode, in nanoseconds.
     *
     * If adaptive latency mode is used (default setting, see \c target_latency),
     * \c min_target_latency and \c max_target_latency define the allowed range
     * for the target latency.
     *
     * You should either set both \c min_target_latency and \c max_target_latency,
     * or keep both zero to use default values.
     */
    unsigned long long min_target_latency;

    /** Maximum latency for adaptive mode, in nanoseconds.
     *
     * If adaptive latency mode is used (default setting, see \c target_latency),
     * \c min_target_latency and \c max_target_latency define the allowed range
     * for the target latency.
     *
     * You should either set both \c min_target_latency and \c max_target_latency,
     * or keep both zero to use default values.
     */
    unsigned long long max_target_latency;

    /** Timeout for the lack of playback, in nanoseconds.
     *
     * If there is no playback during this period, receiver terminates connection to
     * to sender (but sender may reconnect).
     *
     * This mechanism allows to detect dead, hanging, or incompatible clients that
     * generate unparseable packets.
     *
     * If zero, default value is used. If negative, the check is disabled.
     */
    long long no_playback_timeout;

    /** Timeout for choppy playback, in nanoseconds.
     *
     * If there is constant stuttering during this period, receiver terminates connection
     * to sender (but sender may reconnect).
     *
     * This mechanism allows to detect situations when playback continues but there
     * are frequent glitches, for example because there is a high ratio of late packets.
     *
     * If zero, default value is used. If negative, the check is disabled.
     */
    long long choppy_playback_timeout;
} roc_receiver_config;

/** Interface configuration.
 *
 * Sender and receiver can have multiple slots (\ref roc_slot), and each slot can
 * be bound or connected to multiple interfaces (\ref roc_interface).
 *
 * Each such interface has its own configuration, defined by this struct.
 *
 * For all fields, zero value means "use default". If you want to set all options
 * to default values, you can memset() this struct with zeros.
 *
 * \see roc_sender_configure(), roc_receiver_configure().
 */
typedef struct roc_interface_config {
    /** Outgoing IP address.
     *
     * If non-empty, explicitly identifies the OS network interface, by its IP address,
     * from which to send outgoing packets. If NULL, the network interface is selected
     * automatically by the OS, depending on the address of remote endpoint.
     *
     * For example, if eth0 has IP address "192.168.0.1", then setting outgoing address
     * to "192.168.0.1" will force usage of eth0 interface.
     *
     * Setting it to `0.0.0.0` (for IPv4) or to `::` (for IPv6) gives the same effect
     * as if it was NULL.
     *
     * By default, empty.
     */
    char outgoing_address[48];

    /** Multicast group IP address.
     *
     * Multicast group should be set only when binding or connecting interface to an
     * endpoint with multicast IP address. If present, it defines an IP address of the
     * OS network interface on which to join the multicast group. If not present, no
     * multicast group is joined.
     *
     * It's allowed to receive multicast traffic from only those OS network interfaces,
     * on which the process has joined the multicast group. When using multicast, the
     * user should either set this field, or join multicast group manually using
     * OS-specific APIs.
     *
     * It is allowed to set multicast group to `0.0.0.0` (for IPv4) or to `::` (for IPv6),
     * to be able to receive multicast traffic from all available interfaces. However,
     * this may not be desirable for security reasons.
     *
     * By default, empty.
     */
    char multicast_group[48];

    /** Socket address reuse flag.
     *
     * When true (non-zero), SO_REUSEADDR is enabled for socket, regardless of socket
     * type, unless binding to ephemeral port (when port is set to zero).
     *
     * When false (zero), SO_REUSEADDR is enabled for socket only if it has multicast
     * type, unless binding to ephemeral port (when port is set to zero).
     *
     * For TCP-based protocols, SO_REUSEADDR allows immediate reuse of recently closed
     * socket in TIME_WAIT state, which may be useful you want to be able to restart
     * server quickly.
     *
     * For UDP-based protocols, SO_REUSEADDR allows multiple processes to bind to the
     * same address, which may be useful if you're using socket activation mechanism.
     *
     * By default, false.
     */
    unsigned int reuse_address;
} roc_interface_config;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_CONFIG_H_
