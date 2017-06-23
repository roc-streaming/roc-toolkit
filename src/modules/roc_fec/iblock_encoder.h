/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/iblock_encoder.h
//! @brief FEC block encoder interface.

#ifndef ROC_FEC_IBLOCK_ENCODER_H_
#define ROC_FEC_IBLOCK_ENCODER_H_

#include "roc_core/byte_buffer.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace fec {

//! FEC block encoder interface.
class IBlockEncoder {
public:
    virtual ~IBlockEncoder();

    //! Store data buffer to current block at given position.
    virtual void write(size_t index, const core::IByteBufferConstSlice& buffer) = 0;

    //! Finish writing data buffers for current block.
    //! @remarks
    //!  Calculates FEC buffers from previously added data buffers. After this
    //!  call, read() can be used to retreive calculated FEC buffers.
    virtual void commit() = 0;

    //! Retreive calculated FEC buffer at given position.
    virtual core::IByteBufferConstSlice read(size_t index) = 0;

    //! Reset state and start next block.
    virtual void reset() = 0;

    //! Returns the number of source packets in block.
    virtual size_t n_source_packets() const = 0;

    //! Returns the number of repair packets in block.
    virtual size_t n_repair_packets() const = 0;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_IBLOCK_ENCODER_H_
