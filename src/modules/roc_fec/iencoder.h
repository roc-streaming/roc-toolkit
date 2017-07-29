/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/iencoder.h
//! @brief FEC block encoder interface.

#ifndef ROC_FEC_IENCODER_H_
#define ROC_FEC_IENCODER_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace fec {

//! FEC block encoder interface.
class IEncoder {
public:
    virtual ~IEncoder();

    //! Get buffer alignment requirement.
    virtual size_t alignment() const = 0;

    //! Store source or repair packet buffer for current block.
    virtual void set(size_t index, const core::Slice<uint8_t>& buffer) = 0;

    //! Fill all repair packets in current block.
    virtual void commit() = 0;

    //! Reset current block.
    virtual void reset() = 0;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_IENCODER_H_
