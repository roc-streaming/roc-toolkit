/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/delayed_reader.h
//! @brief Delayed reader.

#ifndef ROC_PACKET_DELAYED_READER_H_
#define ROC_PACKET_DELAYED_READER_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/ireader.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! Delayed reader.
//!
//! Delays read of the first packet in stream for the configured duration.
//!
//! Assumes that packets arrive at constant rate, and pipeline performs read
//! from delayed reader at the same rate (in average).
//!
//! Operation is split into three stages:
//!
//!   1. Loading: reads packets from incoming queue and accumulates them in
//!      delay queue. Doesn't return packets to pipeline. This stage lasts
//!      until target delay is accumulated. By the end of this stage,
//!      incoming queue length is zero, delay queue length is target delay,
//!      and pipeline is ahead of the last packet in queue by target delay.
//!
//!   2. Unloading: returns packets from delay queue until it becomes empty.
//!      Doesn't read packets from incoming queue. By the end of this stage,
//!      incoming queue length is target delay, delay queue length is zero,
//!      and pipeline is ahead of the last packet in queue by target delay.
//!
//!   3. Forwarding: just forwards packets from incoming queue and doesn't
//!      use delay queue anymore. Incoming queue length remains equal to
//!      target delay, given that packets packets are arriving and read
//!      at the same rate.
class DelayedReader : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is used to read packets from incoming queue
    //!  - @p target_delay is the delay to insert before first packet
    //!  - @p sample_spec is the specifications of incoming packets
    DelayedReader(IReader& reader,
                  core::nanoseconds_t target_delay,
                  const audio::SampleSpec& sample_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read packet.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr& packet,
                                                       PacketReadMode mode);

private:
    status::StatusCode load_queue_();
    stream_timestamp_t calc_queue_duration_() const;

    IReader& reader_;

    SortedQueue delay_queue_;
    stream_timestamp_t delay_;

    bool loaded_;
    bool unloaded_;

    const audio::SampleSpec sample_spec_;

    status::StatusCode init_status_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_DELAYED_READER_H_
