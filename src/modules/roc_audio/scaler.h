/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/scaler.h
//! @brief Scaler.

#ifndef ROC_AUDIO_SCALER_H_
#define ROC_AUDIO_SCALER_H_

#include "roc_config/config.h"

#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/timer.h"

#include "roc_packet/imonitor.h"
#include "roc_packet/ipacket.h"
#include "roc_packet/ipacket_reader.h"
#include "roc_packet/packet_queue.h"

#include "roc_audio/freq_estimator.h"
#include "roc_audio/resampler.h"

namespace roc {
namespace audio {

//! Scaler.
//! @remarks
//!  Monitors queue size, passes it to FreqEstimator to recompute scaling,
//!  and passes updated scaling to connected resamplers.
class Scaler : public packet::IMonitor,
               public packet::IPacketReader,
               public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input packet reader; packets from @p reader
    //!    are returned from read();
    //!  - @p queue is received packet queue used to calculate number
    //!    of pending samples in stream; it may be or may not be the same
    //!    object as @p reader.
    Scaler(packet::IPacketReader& reader,
           packet::PacketQueue const& queue,
           packet::timestamp_t aim_queue_size = ROC_CONFIG_DEFAULT_SESSION_LATENCY);

    //! Update stream.
    //! @remarks
    //!  Calculates scaling and sets it to all added resamplers.
    //! @returns
    //!  false if resampler reports scaling is out of bounds and rendering
    //!  can't be continued.
    virtual bool update();

    //! Read next packet.
    //! @remarks
    //!  updates queue size and returns next packet from input reader.
    virtual packet::IPacketConstPtr read();

    //! Add resampler.
    void add_resampler(Resampler&);

private:
    enum { MaxChannels = ROC_CONFIG_MAX_CHANNELS };

    packet::timestamp_t queue_size_() const;

    void update_packet_(packet::IPacketConstPtr& prev,
                        const packet::IPacketConstPtr& next);

    packet::IPacketReader& reader_;
    packet::PacketQueue const& queue_;
    packet::timestamp_t aim_queue_size_;

    packet::IPacketConstPtr head_;
    packet::IPacketConstPtr tail_;

    FreqEstimator freq_estimator_;

    core::Array<Resampler*, MaxChannels> resamplers_;
    core::Timer timer_;

    bool started_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SCALER_H_
