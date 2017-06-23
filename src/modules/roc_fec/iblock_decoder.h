/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/iblock_decoder.h
//! @brief FEC block decoder interface.

#ifndef ROC_FEC_IBLOCK_DECODER_H_
#define ROC_FEC_IBLOCK_DECODER_H_

#include "roc_core/byte_buffer.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace fec {

//! FEC block decoder interface.
class IBlockDecoder {
public:
    virtual ~IBlockDecoder();

    //! Store encoded buffer to current block at given position.
    virtual void write(size_t index, const core::IByteBufferConstSlice& buffer) = 0;

    //! Repair data buffer at given position of current block.
    virtual core::IByteBufferConstSlice repair(size_t index) = 0;

    //! Reset state and start next block.
    virtual void reset() = 0;

    //! Returns the number of source packets in block.
    virtual size_t n_source_packets() const = 0;

    //! Returns the number of repair packets in block.
    virtual size_t n_repair_packets() const = 0;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_IBLOCK_DECODER_H_
