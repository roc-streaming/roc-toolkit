/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/headers.h
//! @brief RTP header.

#ifndef ROC_RTP_HEADERS_H_
#define ROC_RTP_HEADERS_H_

#include "roc_core/attributes.h"
#include "roc_core/byte_order.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace rtp {

//! RTP protocol version.
enum RTP_Version {
    RTP_V2 = 2 //!< RTP version 2.
};

//! RTP payload type.
enum RTP_PayloadType {
    RTP_PT_L16_STEREO = 10, //!< Audio, 16-bit samples, 2 channels, 44100 Hz.
    RTP_PT_L16_MONO = 11    //!< Audio, 16-bit samples, 1 channel, 44100 Hz.
};

//! RTP header.
//! @remarks
//!  Contains fixed size part of 12 bytes and variable size CSRC array.
//!
//! @code
//!    0             1               2               3               4
//!    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   |                           timestamp                           |
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   |                             SSRC                              |
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   |                             CSRC                              |
//!   |                             ....                              |
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
class ROC_ATTR_PACKED RTP_Header {
private:
    enum {
        //! @name RTP protocol version.
        // @{
        FLAG_VERSION_SHIFT = 6,
        FLAG_VERSION_MASK = 0x3,
        // @}

        //! @name RTP padding flag.
        //! @remarks
        //!  If this flag is set, packet contains padding at the end. Last byte
        //!  contains padding length.
        // @{
        FLAG_PADDING_SHIFT = 5,
        FLAG_PADDING_MASK = 0x1,
        // @}

        //! @name RTP extension header flag.
        //! @remarks
        //!  If this flag is set, packet contains extension header between main
        //!  header and payload.
        // @{
        FLAG_EXTENSION_SHIFT = 4,
        FLAG_EXTENSION_MASK = 0x1,
        // @}

        //! @name Number of CSRC items at the end of RTP header.
        // @{
        FLAG_CSRC_SHIFT = 0,
        FLAG_CSRC_MASK = 0xf,
        // @}

        //! @name RTP marker bit.
        //! @remarks
        //!  Semantics of marker bit may vary and is defined by profile in use.
        // @{
        MPT_MARKER_SHIFT = 7,
        MPT_MARKER_MASK = 0x1,
        // @}

        //! @name RTP payload type.
        // @{
        MPT_PAYLOAD_TYPE_SHIFT = 0,
        MPT_PAYLOAD_TYPE_MASK = 0x7f
        // @}
    };

    //! Packed flags (FLAG_*).
    uint8_t flags_;

    //! Packed marker and payload type fields (MPT_*).
    uint8_t mpt_;

    //! Sequence number.
    uint16_t seqnum_;

    //! Timestamp.
    uint32_t timestamp_;

    //! Stream identifiers (SSRC and zero or more CSRC).
    uint32_t ssrc_[1];

public:
    //! Get header size in bytes.
    uint32_t header_size() const {
        roc_panic_if(sizeof(*this) != 12);
        return sizeof(*this) + num_csrc() * sizeof(uint32_t);
    }

    //! Clear header.
    void clear() {
        memset(this, 0, sizeof(*this));
    }

    //! Get version.
    uint8_t version() const {
        return ((flags_ >> FLAG_VERSION_SHIFT) & FLAG_VERSION_MASK);
    }

    //! Set version.
    void set_version(RTP_Version v) {
        roc_panic_if((v & FLAG_VERSION_MASK) != v);
        flags_ &= ~(FLAG_VERSION_MASK << FLAG_VERSION_SHIFT);
        flags_ |= (v << FLAG_VERSION_SHIFT);
    }

    //! Get padding flag.
    bool has_padding() const {
        return (flags_ & (FLAG_PADDING_MASK << FLAG_PADDING_SHIFT));
    }

    //! Get extension flag.
    bool has_extension() const {
        return (flags_ & (FLAG_EXTENSION_MASK << FLAG_EXTENSION_SHIFT));
    }

    //! Get CSRC array size.
    uint8_t num_csrc() const {
        return ((flags_ >> FLAG_CSRC_SHIFT) & FLAG_CSRC_MASK);
    }

    //! Get payload type.
    uint8_t payload_type() const {
        return ((mpt_ >> MPT_PAYLOAD_TYPE_SHIFT) & MPT_PAYLOAD_TYPE_MASK);
    }

    //! Set payload type.
    void set_payload_type(uint8_t pt) {
        roc_panic_if((pt & MPT_PAYLOAD_TYPE_MASK) != pt);
        mpt_ &= ~(MPT_PAYLOAD_TYPE_MASK << MPT_PAYLOAD_TYPE_SHIFT);
        mpt_ |= (pt << MPT_PAYLOAD_TYPE_SHIFT);
    }

    //! Get marker bit.
    bool marker() const {
        return (mpt_ & (MPT_MARKER_MASK << MPT_MARKER_SHIFT));
    }

    //! Set marker bit.
    void set_marker(bool m) {
        mpt_ &= ~(MPT_MARKER_MASK << MPT_MARKER_SHIFT);
        mpt_ |= ((!!m) << MPT_MARKER_SHIFT);
    }

    //! Get sequence number.
    uint16_t seqnum() const {
        return ROC_NTOH_16(seqnum_);
    }

    //! Set sequence number.
    void set_seqnum(uint16_t sn) {
        seqnum_ = ROC_HTON_16(sn);
    }

    //! Get timestamp.
    uint32_t timestamp() const {
        return ROC_NTOH_32(timestamp_);
    }

    //! Set timestamp.
    void set_timestamp(uint32_t ts) {
        timestamp_ = ROC_HTON_32(ts);
    }

    //! Get SSRC.
    uint32_t ssrc() const {
        return ROC_NTOH_32(ssrc_[0]);
    }

    //! Set SSRC.
    void set_ssrc(uint32_t s) {
        ssrc_[0] = ROC_HTON_32(s);
    }

    //! Get CSRC.
    uint32_t csrc(size_t index) const {
        roc_panic_if(index >= num_csrc());
        return ROC_NTOH_32(ssrc_[index + 1]);
    }
};

//! RTP extension header.
//! @remarks
//!  Extension contains fixed size header of 4 bytes followed by variable
//!  length data.
//!
//! @code
//!    0             1               2               3               4
//!    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   |             type              |           length              |
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   |                        extension data                         |
//!   |                             ....                              |
//!   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
class ROC_ATTR_PACKED RTP_ExtentionHeader {
private:
    //! Extenson type.
    uint16_t type_;

    //! Number of 32-bit words in data following extension header.
    uint16_t len_;

public:
    //! Get extension type.
    uint16_t type() const {
        return ROC_NTOH_16(type_);
    }

    //! Get extension data size in bytes (without extension header itself).
    uint32_t data_size() const {
        return (uint32_t(ROC_NTOH_16(len_)) << 2);
    }
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_HEADERS_H_
