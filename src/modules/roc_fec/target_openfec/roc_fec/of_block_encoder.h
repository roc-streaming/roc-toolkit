/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/target_openfec/roc_fec/of_block_encoder.h
//! @brief Implementation of IBlockEncoder using OpenFEC library.

#ifndef ROC_FEC_LDPC_BLOCK_ENCODER_H_
#define ROC_FEC_LDPC_BLOCK_ENCODER_H_

#include "roc_config/config.h"
#include "roc_core/noncopyable.h"
#include "roc_core/byte_buffer.h"
#include "roc_core/array.h"
#include "roc_datagram/default_buffer_composer.h"
#include "roc_packet/units.h"
#include "roc_fec/iblock_encoder.h"

extern "C" {
#include <of_openfec_api.h>
}

#ifndef OF_USE_ENCODER
#error OF_USE_ENCODER undefined
#endif

#ifndef OF_USE_LDPC_STAIRCASE_CODEC
#error OF_USE_LDPC_STAIRCASE_CODEC undefined
#endif

namespace roc {
namespace fec {

//! Implementation of IBlockEncoder using OpenFEC library.
class OF_BlockEncoder : public IBlockEncoder, public core::NonCopyable<> {
public:
    //! Construct.
    explicit OF_BlockEncoder(
        core::IByteBufferComposer& composer = datagram::default_buffer_composer());

    virtual ~OF_BlockEncoder();

    //! Store data buffer to current block at given position.
    virtual void write(size_t index, const core::IByteBufferConstSlice& buffer);

    //! Finish writing data buffers for current block.
    virtual void commit();

    //! Retreive calculated FEC buffer at given position.
    virtual core::IByteBufferConstSlice read(size_t index);

    //! Reset state and start next block.
    virtual void reset();

private:
    static const of_codec_id_t codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;

    static const size_t N_DATA_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_DATA_PACKETS;
    static const size_t N_FEC_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_REDUNDANT_PACKETS;

    of_session_t* of_inst_;
    of_parameters_t *of_inst_params_;
    union {
        of_ldpc_parameters ldpc_params_;
        of_rs_2_m_parameters_t rs_params_;
    } fec_codec_params_;

    core::IByteBufferComposer& composer_;

    core::Array<void*, N_DATA_PACKETS + N_FEC_PACKETS> sym_tab_;
    core::Array<core::IByteBufferConstSlice, N_DATA_PACKETS + N_FEC_PACKETS> buffers_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_LDPC_BLOCK_ENCODER_H_
