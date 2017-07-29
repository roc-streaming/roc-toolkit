/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/idecoder.h
//! @brief FEC block decoder interface.

#ifndef ROC_FEC_IDECODER_H_
#define ROC_FEC_IDECODER_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace fec {

//! FEC block decoder interface.
class IDecoder {
public:
    virtual ~IDecoder();

    //! Store source or repair packet buffer for current block.
    virtual void set(size_t index, const core::Slice<uint8_t>& buffer) = 0;

    //! Repair source packet buffer.
    virtual core::Slice<uint8_t> repair(size_t index) = 0;

    //! Reset current block.
    virtual void reset() = 0;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_IDECODER_H_
