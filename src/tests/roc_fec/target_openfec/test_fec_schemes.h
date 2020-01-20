/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_FEC_TARGET_OPENFEC_TEST_FEC_SCHEMES_H_
#define ROC_FEC_TARGET_OPENFEC_TEST_FEC_SCHEMES_H_

#include "roc_core/helpers.h"
#include "roc_packet/fec.h"

namespace roc {
namespace fec {

namespace {

packet::FecScheme Test_fec_schemes[] = { packet::FEC_ReedSolomon_M8,
                                         packet::FEC_LDPC_Staircase };

const size_t Test_n_fec_schemes = ROC_ARRAY_SIZE(Test_fec_schemes);

} // namespace

} // namespace fec
} // namespace roc

#endif // ROC_FEC_TARGET_OPENFEC_TEST_FEC_SCHEMES_H_
