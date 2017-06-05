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

#ifndef ROC_FEC_OF_BLOCK_ENCODER_H_
#define ROC_FEC_OF_BLOCK_ENCODER_H_

#include "roc_config/config.h"
#include "roc_core/noncopyable.h"
#include "roc_core/byte_buffer.h"
#include "roc_core/array.h"
#include "roc_datagram/default_buffer_composer.h"
#include "roc_packet/units.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/fec_config.h"

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
class OFBlockEncoder : public IBlockEncoder, public core::NonCopyable<> {
public:
    //! Construct.
    explicit OFBlockEncoder(const FECConfig &fec_config,
        core::IByteBufferComposer& composer = datagram::default_buffer_composer());
    virtual ~OFBlockEncoder();

    //! Store data buffer to current block at given position.
    virtual void write(size_t index, const core::IByteBufferConstSlice& buffer);

    //! Finish writing data buffers for current block.
    virtual void commit();

    //! Retreive calculated FEC buffer at given position.
    virtual core::IByteBufferConstSlice read(size_t index);

    //! Reset state and start next block.
    virtual void reset();

    //! Returns the number of data packets.
    virtual size_t n_data_packets() const;

    //! Returns the number of FEC packets.
    virtual size_t n_fec_packets() const;

private:
    const FECConfig &fec_config_;
    of_codec_id_t codec_id_;

    //! Shortcut for fec_config_.n_source_packets.
    const size_t n_data_packets_;
    //! Shortcut for fec_config_.n_repair_packets.
    const size_t n_fec_packets_;

    of_session_t* of_inst_;
    of_parameters_t* of_inst_params_;
    union {
        of_ldpc_parameters ldpc_params_;
        of_rs_2_m_parameters_t rs_params_;
    } fec_codec_params_;

    core::IByteBufferComposer& composer_;

    //! Shortcut for maximal number of packets we should store.
    static const size_t max_n_packets_ = ROC_CONFIG_MAX_FEC_BLOCK_DATA_PACKETS + ROC_CONFIG_MAX_FEC_BLOCK_REDUNDANT_PACKETS;
    core::Array<void*, max_n_packets_> sym_tab_;
    core::Array<core::IByteBufferConstSlice, max_n_packets_> buffers_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_OF_BLOCK_ENCODER_H_
