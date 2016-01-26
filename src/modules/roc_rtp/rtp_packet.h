/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/rtp_packet.h
//! @brief RTP packet.

#ifndef ROC_RTP_RTP_PACKET_H_
#define ROC_RTP_RTP_PACKET_H_

#include "roc_core/stddefs.h"
#include "roc_core/byte_buffer.h"
#include "roc_packet/units.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

//! RTP packet.
class RTP_Packet {
public:
    RTP_Packet();

    //! Fill empty packet.
    void compose(const core::IByteBufferPtr& buffer);

    //! Parse packet from buffer.
    bool parse(const core::IByteBufferConstSlice& buffer);

    //! Get raw data.
    const core::IByteBufferConstSlice& raw_data() const;

    //! Get RTP header.
    const RTP_Header& header() const;

    //! Get RTP header.
    RTP_Header& header();

    //! Get RTP extension header.
    const RTP_ExtentionHeader* ext_header() const;

    //! Get RTP payload.
    core::IByteBufferConstSlice payload() const;

    //! Get RTP payload.
    core::IByteBufferSlice payload();

    //! Set payload size in bytes.
    void set_payload_size(size_t size);

private:
    core::IByteBuffer& mut_buffer_();

    size_t payload_off_;
    size_t payload_size_;

    core::IByteBufferConstSlice buffer_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_RTP_PACKET_H_
