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

#ifndef ROC_FEC_OF_BLOCK_DECODER_H_
#define ROC_FEC_OF_BLOCK_DECODER_H_

#include "roc_config/config.h"
#include "roc_core/array.h"
#include "roc_core/byte_buffer.h"
#include "roc_core/noncopyable.h"
#include "roc_datagram/default_buffer_composer.h"
#include "roc_fec/config.h"
#include "roc_fec/iblock_decoder.h"
#include "roc_packet/units.h"

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
class OFBlockDecoder : public IBlockDecoder, public core::NonCopyable<> {
public:
    //! Construct.
    explicit OFBlockDecoder(
        const Config& config,
        core::IByteBufferComposer& composer = datagram::default_buffer_composer());

    virtual ~OFBlockDecoder();

    //! Store encoded buffer to current block at given position.
    virtual void write(size_t index, const core::IByteBufferConstSlice& buffer);

    //! Repair data buffer at given position of current block.
    virtual core::IByteBufferConstSlice repair(size_t index);

    //! Reset state and start next block.
    virtual void reset();

    //! Returns the number of source packets in block.
    virtual size_t n_source_packets() const;

    //! Returns the number of repair packets in block.
    virtual size_t n_repair_packets() const;

private:
    void update_();
    void decode_();

    bool has_n_packets_(size_t n_packets) const;
    bool is_optimal_() const;

    void reset_session_();
    void destroy_session_();

    void report_() const;

    void fix_buffer_(size_t index);
    void* make_buffer_(size_t index);

    static void* source_cb_(void* context, uint32_t size, uint32_t index);
    static void* repair_cb_(void* context, uint32_t size, uint32_t index);

    // maximum block size
    static const size_t max_packets_ =
        ROC_CONFIG_MAX_FEC_BLOCK_SOURCE_PACKETS + ROC_CONFIG_MAX_FEC_BLOCK_REPAIR_PACKETS;

    // block size
    const size_t blk_source_packets_;
    const size_t blk_repair_packets_;

    of_codec_id_t codec_id_;
    union {
        of_rs_2_m_parameters_t rs_params_;
        of_ldpc_parameters ldpc_params_;
    } codec_params_;

    // session is recreated for every new block
    of_session_t* of_sess_;
    of_parameters_t* of_sess_params_;

    core::IByteBufferComposer& composer_;

    // received and repaired source and repair packets
    core::Array<core::IByteBufferConstSlice, max_packets_> buff_tab_;

    // data of received and repaired source and repair packets
    // points to buff_tab_[x].data() or to memory allocated by OpenFEC
    core::Array<void*, max_packets_> data_tab_;

    // true if packet is received, false if it's is lost or repaired
    core::Array<bool, max_packets_> recv_tab_;

    bool has_new_packets_;
    bool decoding_finished_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_OF_BLOCK_DECODER_H_
