/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/port.h
//! @brief Port constants.

#ifndef ROC_PIPELINE_PORT_H_
#define ROC_PIPELINE_PORT_H_

namespace roc {
namespace pipeline {

//! Port type.
enum PortType {
    //! Audio source packets.
    Port_AudioSource,

    //! Audio repair packets.
    Port_AudioRepair
};

//! Port protocol.
enum PortProtocol {
    //! Protocol is not set.
    Proto_None,

    //! Bare RTP.
    Proto_RTP,

    //! RTP source packet + FECFRAME Reed-Solomon footer (m=8).
    Proto_RTP_RSm8_Source,

    //! FEC repair packet + FECFRAME Reed-Solomon header (m=8).
    Proto_RSm8_Repair,

    //! RTP source packet + FECFRAME LDPC footer.
    Proto_RTP_LDPC_Source,

    //! FEC repair packet + FECFRAME LDPC header.
    Proto_LDPC_Repair
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_PORT_H_
