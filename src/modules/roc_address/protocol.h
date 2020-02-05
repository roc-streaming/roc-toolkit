/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/protocol.h
//! @brief Protocol ID.

#ifndef ROC_ADDRESS_PROTOCOL_H_
#define ROC_ADDRESS_PROTOCOL_H_

#include "roc_packet/fec.h"

namespace roc {
namespace address {

//! Protocol ID.
enum Protocol {
    //! Protocol is not set.
    Proto_None,

    //! RTSP.
    Proto_RTSP,

    //! Bare RTP.
    Proto_RTP,

    //! RTP source packet + FECFRAME Reed-Solomon footer (m=8).
    Proto_RTP_RS8M_Source,

    //! FEC repair packet + FECFRAME Reed-Solomon header (m=8).
    Proto_RS8M_Repair,

    //! RTP source packet + FECFRAME LDPC footer.
    Proto_RTP_LDPC_Source,

    //! FEC repair packet + FECFRAME LDPC header.
    Proto_LDPC_Repair
};

//! Get string name of the protocol.
const char* proto_to_str(Protocol proto);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_PROTOCOL_H_
