/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/headers.h
//! @brief RTP headers.

#ifndef ROC_RTP_HEADERS_H_
#define ROC_RTP_HEADERS_H_

#include "roc_core/attributes.h"
#include "roc_core/endian.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace rtp {

//! RTP protocol version.
enum Version {
    V2 = 2 //!< RTP version 2.
};

//! RTP payload type.
enum PayloadType {
    PayloadType_L16_Stereo = 10, //!< Audio, 16-bit samples, 2 channels, 44100 Hz.
    PayloadType_L16_Mono = 11    //!< Audio, 16-bit samples, 1 channel, 44100 Hz.
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
ROC_ATTR_PACKED_BEGIN class Header {
private:
    enum {
        //! @name RTP protocol version.
        // @{
        Flag_VersionShift = 6,
        Flag_VersionMask = 0x3,
        // @}

        //! @name RTP padding flag.
        //! @remarks
        //!  If this flag is set, packet contains padding at the end. Last byte
        //!  contains padding length.
        // @{
        Flag_PaddingShift = 5,
        Flag_PaddingMask = 0x1,
        // @}

        //! @name RTP extension header flag.
        //! @remarks
        //!  If this flag is set, packet contains extension header between main
        //!  header and payload.
        // @{
        Flag_ExtensionShift = 4,
        Flag_ExtensionMask = 0x1,
        // @}

        //! @name Number of CSRC items at the end of RTP header.
        // @{
        Flag_CSRCShift = 0,
        Flag_CSRCMask = 0xf,
        // @}

        //! @name RTP marker bit.
        //! @remarks
        //!  Semantics of marker bit may vary and is defined by profile in use.
        // @{
        MPT_MarkerShift = 7,
        MPT_MarkerMask = 0x1,
        // @}

        //! @name RTP payload type.
        // @{
        MPT_PayloadTypeShift = 0,
        MPT_PayloadTypeMask = 0x7f
        // @}
    };

    //! Packed flags (Flag_*).
    uint8_t flags_;

    //! Packed marker and payload type fields (MPT__*).
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
        return ((flags_ >> Flag_VersionShift) & Flag_VersionMask);
    }

    //! Set version.
    void set_version(Version v) {
        roc_panic_if((v & Flag_VersionMask) != v);
        flags_ &= ~(Flag_VersionMask << Flag_VersionShift);
        flags_ |= (v << Flag_VersionShift);
    }

    //! Get padding flag.
    bool has_padding() const {
        return (flags_ & (Flag_PaddingMask << Flag_PaddingShift));
    }

    //! Set padding flag.
    void set_padding(bool v) {
        flags_ &= ~(Flag_PaddingMask << Flag_PaddingShift);
        flags_ |= ((v ? 1 : 0) << Flag_PaddingShift);
    }

    //! Get extension flag.
    bool has_extension() const {
        return (flags_ & (Flag_ExtensionMask << Flag_ExtensionShift));
    }

    //! Get CSRC array size.
    uint8_t num_csrc() const {
        return ((flags_ >> Flag_CSRCShift) & Flag_CSRCMask);
    }

    //! Get payload type.
    uint8_t payload_type() const {
        return ((mpt_ >> MPT_PayloadTypeShift) & MPT_PayloadTypeMask);
    }

    //! Set payload type.
    void set_payload_type(uint8_t pt) {
        roc_panic_if((pt & MPT_PayloadTypeMask) != pt);
        mpt_ &= ~(MPT_PayloadTypeMask << MPT_PayloadTypeShift);
        mpt_ |= (pt << MPT_PayloadTypeShift);
    }

    //! Get marker bit.
    bool marker() const {
        return (mpt_ & (MPT_MarkerMask << MPT_MarkerShift));
    }

    //! Set marker bit.
    void set_marker(bool m) {
        mpt_ &= ~(MPT_MarkerMask << MPT_MarkerShift);
        mpt_ |= ((!!m) << MPT_MarkerShift);
    }

    //! Get sequence number.
    uint16_t seqnum() const {
        return core::ntoh16(seqnum_);
    }

    //! Set sequence number.
    void set_seqnum(uint16_t sn) {
        seqnum_ = core::hton16(sn);
    }

    //! Get timestamp.
    uint32_t timestamp() const {
        return core::ntoh32(timestamp_);
    }

    //! Set timestamp.
    void set_timestamp(uint32_t ts) {
        timestamp_ = core::hton32(ts);
    }

    //! Get SSRC.
    uint32_t ssrc() const {
        return core::ntoh32(ssrc_[0]);
    }

    //! Set SSRC.
    void set_ssrc(uint32_t s) {
        ssrc_[0] = core::hton32(s);
    }

    //! Get CSRC.
    uint32_t csrc(size_t index) const {
        roc_panic_if(index >= num_csrc());
        return core::ntoh32(ssrc_[index + 1]);
    }
} ROC_ATTR_PACKED_END;

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
ROC_ATTR_PACKED_BEGIN class ExtentionHeader {
private:
    //! Extenson type.
    uint16_t type_;

    //! Number of 32-bit words in data following extension header.
    uint16_t len_;

public:
    //! Get extension type.
    uint16_t type() const {
        return core::ntoh16(type_);
    }

    //! Get extension data size in bytes (without extension header itself).
    uint32_t data_size() const {
        return (uint32_t(core::ntoh16(len_)) << 2);
    }
} ROC_ATTR_PACKED_END;

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_HEADERS_H_
