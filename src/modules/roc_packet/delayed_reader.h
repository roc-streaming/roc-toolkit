/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/delayed_reader.h
//! @brief Delayed reader.

#ifndef ROC_PACKET_DELAYED_READER_H_
#define ROC_PACKET_DELAYED_READER_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"
#include "roc_packet/sorted_queue.h"

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
    //!  - @p delay is the delay to insert before first packet
    DelayedReader(IReader& reader, timestamp_t delay);

    //! Read packet.
    virtual PacketPtr read();

private:
    timestamp_t queue_size_() const;

    IReader& reader_;
    SortedQueue queue_;
    timestamp_t delay_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_DELAYED_READER_H_
