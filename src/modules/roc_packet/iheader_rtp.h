/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/iheader_rtp.h
//! @brief RTP header interface.

#ifndef ROC_PACKET_IHEADER_RTP_H_
#define ROC_PACKET_IHEADER_RTP_H_

#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! RTP header interface.
class IHeaderRTP {
public:
    virtual ~IHeaderRTP();

    //! Get packet source ID identifying client stream.
    //! @remarks
    //!  Sequence numbers and timestamp are numbered independently inside
    //!  different client streams.
    virtual source_t source() const = 0;

    //! Set packet source ID.
    virtual void set_source(source_t) = 0;

    //! Get packet sequence number in client stream.
    //! @remarks
    //!  Packets are numbered sequentaly in every stream, starting from some
    //!  random value.
    //! @note
    //!  Seqnum overflow may occur, use ROC_IS_BEFORE() macro to compare them.
    virtual seqnum_t seqnum() const = 0;

    //! Set packet sequence number.
    virtual void set_seqnum(seqnum_t) = 0;

    //! Get packet timestamp.
    //! @remarks
    //!  Timestamp meaning depends on packet type. For audio packet, it may be
    //!  used to define number of first sample in packet.
    //! @note
    //!  Timestamp overflow may occur, use ROC_IS_BEFORE() macro to compare them.
    virtual timestamp_t timestamp() const = 0;

    //! Set packet timestamp.
    virtual void set_timestamp(timestamp_t) = 0;

    //! Get packet rate.
    //! @returns
    //!  Number of timestamp units per second or 0 if timestamp is meaningless
    //!  for this packet.
    virtual size_t rate() const = 0;

    //! Get packet marker bit.
    //! @remarks
    //!  Marker bit meaning depends on packet type.
    virtual bool marker() const = 0;

    //! Set packet marker bit.
    virtual void set_marker(bool) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IHEADER_RTP_H_
