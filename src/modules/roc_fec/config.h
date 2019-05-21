/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/config.h
//! @brief FEC code type options.

#ifndef ROC_FEC_CONFIG_H_
#define ROC_FEC_CONFIG_H_

#include "roc_core/stddefs.h"
#include "roc_packet/fec.h"

namespace roc {
namespace fec {

//! FEC configuration.
struct Config {
    //! FEC scheme.
    packet::FECScheme scheme;

    //! Number of data packets in block.
    size_t n_source_packets;

    //! Number of FEC packets in block.
    size_t n_repair_packets;

    //! Seed for LDPC scheme.
    int32_t ldpc_prng_seed;

    //! N1 parameter LDPC scheme.
    uint8_t ldpc_N1;

    //! Configuration for ReedSolomon scheme.
    uint16_t rs_m;

    //! Maximum allowed source block number jump.
    size_t max_sbn_jump;

    Config()
        : scheme(packet::FEC_None)
        , n_source_packets(20)
        , n_repair_packets(10)
        , ldpc_prng_seed(1297501556)
        , ldpc_N1(7)
        , rs_m(8)
        , max_sbn_jump(100) {
    }
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_CONFIG_H_
