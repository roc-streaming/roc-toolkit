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
//! @remarks
//!  Delays audio packet reader for given amount of samples.
class DelayedReader : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is used to read packets
    //!  - @p sample_spec is the specifications of incoming packets
    DelayedReader(IReader& reader,
                  const audio::SampleSpec& sample_spec);

    //! Check if object was constructed successfully.
    bool is_valid() const;

    //! Read packet.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr&);

    void start();

    //! Check if object was constructed successfully.
    bool is_started() const;

private:
    status::StatusCode fetch_packets_();
    status::StatusCode read_queued_packet_(PacketPtr&);

    stream_timestamp_t queue_size_() const;

    IReader& reader_;
    SortedQueue queue_;

    bool started_;

    const audio::SampleSpec sample_spec_;

    bool valid_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_DELAYED_READER_H_
