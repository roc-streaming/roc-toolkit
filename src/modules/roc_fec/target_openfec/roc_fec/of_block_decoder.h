/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/target_openfec/roc_fec/of_block_decoder.h
//! @brief Implementation of IBlockDecoder using OpenFEC library.

#ifndef ROC_FEC_LDPC_BLOCK_DECODER_H_
#define ROC_FEC_LDPC_BLOCK_DECODER_H_

#include "roc_config/config.h"
#include "roc_core/noncopyable.h"
#include "roc_core/byte_buffer.h"
#include "roc_core/array.h"
#include "roc_datagram/default_buffer_composer.h"
#include "roc_packet/units.h"
#include "roc_fec/iblock_decoder.h"
#include "roc_fec/fec_type_options.h"

extern "C" {
#include <of_openfec_api.h>
}

#ifndef OF_USE_DECODER
#error OF_USE_DECODER undefined
#endif

#ifndef OF_USE_LDPC_STAIRCASE_CODEC
#error OF_USE_LDPC_STAIRCASE_CODEC undefined
#endif

namespace roc {
namespace fec {

//! Implementation of IBlockDecoder using OpenFEC library.
class OF_BlockDecoder : public IBlockDecoder, public core::NonCopyable<> {
public:
    //! Construct.
    explicit OF_BlockDecoder(
        core::IByteBufferComposer& composer = datagram::default_buffer_composer(),
        fec_codec_type_t fec_type = OF_REED_SOLOMON_2_M);

    virtual ~OF_BlockDecoder();

    //! Store encoded buffer to current block at given position.
    virtual void write(size_t index, const core::IByteBufferConstSlice& buffer);

    //! Repair data buffer at given position of current block.
    virtual core::IByteBufferConstSlice repair(size_t index);

    //! Reset state and start next block.
    virtual void reset();

private:
    fec_codec_type_t fec_type_;
    of_codec_id_t codec_id_;

    static const size_t N_DATA_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_DATA_PACKETS;
    static const size_t N_FEC_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_REDUNDANT_PACKETS;

    static void* source_cb_(void* context, uint32_t size, uint32_t index);
    static void* repair_cb_(void* context, uint32_t size, uint32_t index);

    void report_();

    void* make_buffer_(const size_t index);

    of_session_t* of_inst_;
    bool of_inst_inited_;
    of_parameters_t *of_inst_params_;

    union {
        of_ldpc_parameters ldpc_params_;
        of_rs_2_m_parameters_t rs_params_;
    } fec_codec_params_;

    core::IByteBufferComposer& composer_;

    core::Array<core::IByteBufferConstSlice, N_DATA_PACKETS + N_FEC_PACKETS> buffers_;
    core::Array<void*, N_DATA_PACKETS + N_FEC_PACKETS> sym_tab_;
    core::Array<bool, N_DATA_PACKETS + N_FEC_PACKETS> received_;

    bool defecation_attempted_;

    size_t packets_rcvd_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_LDPC_BLOCK_DECODER_H_
