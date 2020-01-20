/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/codec_config.h
//! @brief FEC codec parameters.

#ifndef ROC_FEC_CODEC_CONFIG_H_
#define ROC_FEC_CODEC_CONFIG_H_

#include "roc_core/stddefs.h"
#include "roc_packet/fec.h"

namespace roc {
namespace fec {

//! FEC codec parameters.
struct CodecConfig {
    //! FEC scheme.
    packet::FecScheme scheme;

    //! Seed for LDPC scheme.
    int32_t ldpc_prng_seed;

    //! N1 parameter LDPC scheme.
    uint8_t ldpc_N1;

    //! Configuration for ReedSolomon scheme.
    uint16_t rs_m;

    CodecConfig()
        : scheme(packet::FEC_None)
        , ldpc_prng_seed(1297501556)
        , ldpc_N1(7)
        , rs_m(8) {
    }
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_CODEC_CONFIG_H_
