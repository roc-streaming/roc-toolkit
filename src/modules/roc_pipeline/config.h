/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/config.h
//! @brief Pipeline config.

#ifndef ROC_PIPELINE_CONFIG_H_
#define ROC_PIPELINE_CONFIG_H_

#include "roc_audio/resampler.h"
#include "roc_core/stddefs.h"
#include "roc_fec/config.h"
#include "roc_packet/units.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/validator.h"

namespace roc {
namespace pipeline {

//! Defaults.
enum {
    //! Number of samples per second.
    DefaultSampleRate = 44100,

    //! Channel mask.
    DefaultChannelMask = 0x3,

    //! Number of samples per packet per channel.
    DefaultPacketSize = 320
};

//! Protocol identifier.
enum Protocol {
    //! Bare RTP.
    Proto_RTP,

    //! RTP source packet + FECFRAME Reed-Solomon footer (m=8).
    Proto_RTP_RSm8_Source,

    //! FEC repair packet + FECFRAME Reed-Solomon header (m=8).
    Proto_RSm8_Repair,

    //! RTP source packet + FECFRAME LDPC footer.
    Proto_RTP_LDPC_Source,

    //! FEC repair packet + FECFRAME LDPC header.
    Proto_LDPC_Repair
};

//! Port parameters.
//! @remarks
//!  On receiver, defines a listened port parameters. On sender,
//!  defines a destination port parameters.
struct PortConfig {
    //! Port address.
    packet::Address address;

    //! Port protocol.
    Protocol protocol;

    PortConfig()
        : protocol() {
    }
};

//! Session parameters.
//! @remarks
//!  Defines per-session parameters on the receiver side.
struct SessionConfig {
    //! Channel mask.
    packet::channel_mask_t channels;

    //! Number of samples per packet per channel.
    size_t samples_per_packet;

    //! Target latency, number of samples.
    packet::timestamp_t latency;

    //! Session timeout, number of samples.
    //! @remarks
    //!  If there are no new packets during this period, the session is terminated.
    packet::timestamp_t timeout;

    //! RTP payload type for audio packets.
    rtp::PayloadType payload_type;

    //! FEC scheme parameters.
    fec::Config fec;

    //! RTP validator parameters.
    rtp::ValidatorConfig validator;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! FreqEstimator update interval, number of samples
    packet::timestamp_t fe_update_interval;

    //! Perform resampling to to compensate sender and receiver frequency difference.
    bool resampling;

    //! Insert weird beeps instead of silence on packet loss.
    bool beep;

    SessionConfig()
        : channels(DefaultChannelMask)
        , samples_per_packet(DefaultPacketSize)
        , latency(DefaultPacketSize * 27)
        , timeout(DefaultSampleRate * 2)
        , payload_type(rtp::PayloadType_L16_Stereo)
        , fe_update_interval(4096)
        , resampling(false)
        , beep(false) {
    }
};

//! Receiver parameters.
struct ReceiverConfig {
    //! Default parameters for session.
    SessionConfig default_session;

    //! Sample rate, number of samples for all channels per second.
    size_t sample_rate;

    //! Channel mask.
    packet::channel_mask_t channels;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool timing;

    ReceiverConfig()
        : sample_rate(DefaultSampleRate)
        , channels(DefaultChannelMask)
        , timing(false) {
    }
};

//! Sender parameters.
struct SenderConfig {
    //! Parameters for the port from which source packets are sent.
    PortConfig source_port;

    //! Parameters for the port from which repair packets are sent.
    PortConfig repair_port;

    //! Sample rate, number of samples for all channels per second.
    size_t sample_rate;

    //! Channel mask.
    packet::channel_mask_t channels;

    //! Number of samples per packet per channel.
    size_t samples_per_packet;

    //! Interleave packets.
    bool interleaving;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool timing;

    //! RTP payload type for audio packets.
    rtp::PayloadType payload_type;

    //! FEC scheme parameters.
    fec::Config fec;

    SenderConfig()
        : sample_rate(DefaultSampleRate)
        , channels(DefaultChannelMask)
        , samples_per_packet(DefaultPacketSize)
        , interleaving(false)
        , timing(false)
        , payload_type(rtp::PayloadType_L16_Stereo) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
