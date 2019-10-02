/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/endpoint_enums.h
//! @brief Network endpoint enums.

#ifndef ROC_ADDRESS_ENDPOINT_ENUMS_H_
#define ROC_ADDRESS_ENDPOINT_ENUMS_H_

namespace roc {
namespace address {

//! Network endpoint type.
enum EndpointType {
    //! Session initiation and control.
    EndType_Session,

    //! Audio source packets.
    EndType_AudioSource,

    //! Audio repair packets.
    EndType_AudioRepair
};

//! Network endpoint protocol.
enum EndpointProtocol {
    //! Protocol is not set.
    EndProto_None,

    //! RTSP.
    EndProto_RTSP,

    //! Bare RTP.
    EndProto_RTP,

    //! RTP source packet + FECFRAME Reed-Solomon footer (m=8).
    EndProto_RTP_RS8M_Source,

    //! FEC repair packet + FECFRAME Reed-Solomon header (m=8).
    EndProto_RS8M_Repair,

    //! RTP source packet + FECFRAME LDPC footer.
    EndProto_RTP_LDPC_Source,

    //! FEC repair packet + FECFRAME LDPC header.
    EndProto_LDPC_Repair
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_ENDPOINT_ENUMS_H_
