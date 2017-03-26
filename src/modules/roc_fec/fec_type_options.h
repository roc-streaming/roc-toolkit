/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/fec_type_options.h
//! @brief FEC code type options.

#ifndef ROC_FEC_TYPE_OPTIONS_H_
#define ROC_FEC_TYPE_OPTIONS_H_

namespace roc {
namespace fec {

/** \brief Available options of FEC code type. */
typedef enum {
    //! OpenFEC Reed-Solomon.
    OF_REED_SOLOMON_2_M,
    //! OpenFEC LDPC Staircase.
    OF_LDPC_STAIRCASE,
    //! Maximum for iterating through the enums.
    FEC_TYPE_UNDEFINED
} fec_codec_type_t;

} // namespace fec
} // namespace roc

#endif // ROC_FEC_TYPE_OPTIONS_H_
