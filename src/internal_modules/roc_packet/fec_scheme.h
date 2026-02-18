/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/fec_scheme.h
//! @brief FEC scheme.

#ifndef ROC_PACKET_FEC_SCHEME_H_
#define ROC_PACKET_FEC_SCHEME_H_

namespace roc {
namespace packet {

//! FECFRAME scheme.
enum FecScheme {
    //! No FEC.
    FEC_None,

    //! Reed-Solomon (m=8).
    FEC_ReedSolomon_M8,

    //! LDPC-Staircase.
    FEC_LDPC_Staircase
};

//! FEC scheme to string.
const char* fec_scheme_to_str(FecScheme scheme);

//! FEC scheme from string.
FecScheme fec_scheme_from_str(const char* str);

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_FEC_SCHEME_H_
