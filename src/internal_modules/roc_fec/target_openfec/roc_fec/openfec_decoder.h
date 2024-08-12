/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/target_openfec/roc_fec/openfec_decoder.h
//! @brief Decoder implementation using OpenFEC library.

#ifndef ROC_FEC_OPENFEC_DECODER_H_
#define ROC_FEC_OPENFEC_DECODER_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_fec/codec_config.h"
#include "roc_fec/iblock_decoder.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/units.h"

extern "C" {
#include <of_openfec_api.h>
}

namespace roc {
namespace fec {

//! Decoder implementation using OpenFEC library.
class OpenfecDecoder : public IBlockDecoder, public core::NonCopyable<> {
public:
    //! Initialize.
    OpenfecDecoder(const CodecConfig& config,
                   packet::PacketFactory& packet_factory,
                   core::IArena& arena);

    virtual ~OpenfecDecoder();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Get the maximum number of encoding symbols for the scheme being used.
    virtual size_t max_block_length() const;

    //! Start block.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    begin_block(size_t sblen, size_t rblen, size_t payload_size);

    //! Store source or repair packet buffer for current block.
    virtual void set_buffer(size_t index, const core::Slice<uint8_t>& buffer);

    //! Repair source packet buffer.
    virtual core::Slice<uint8_t> repair_buffer(size_t index);

    //! Finish block.
    virtual void end_block();

private:
    void update_session_params_(size_t sblen, size_t rblen, size_t payload_size);

    void reset_tabs_();
    bool resize_tabs_(size_t size);

    void update_();
    void decode_();

    bool has_n_packets_(size_t n_packets) const;
    bool is_optimal_() const;

    void reset_session_();
    void destroy_session_();

    void report_();

    void fix_buffer_(size_t index);
    void* make_buffer_(size_t index);

    static void* source_cb_(void* context, uint32_t size, uint32_t index);
    static void* repair_cb_(void* context, uint32_t size, uint32_t index);

    size_t sblen_;
    size_t rblen_;
    size_t payload_size_;
    size_t max_index_;

    of_codec_id_t codec_id_;
    union {
#ifdef OF_USE_REED_SOLOMON_2_M_CODEC
        of_rs_2_m_parameters_t rs_params_;
#endif
#ifdef OF_USE_LDPC_STAIRCASE_CODEC
        of_ldpc_parameters ldpc_params_;
#endif
    } codec_params_;

    // session is recreated for every new block
    of_session_t* of_sess_;
    of_parameters_t* of_sess_params_;

    packet::PacketFactory& packet_factory_;

    // received and repaired source and repair packets
    core::Array<core::Slice<uint8_t> > buff_tab_;

    // data of received and repaired source and repair packets
    // points to buff_tab_[x].data() or to memory allocated by OpenFEC
    core::Array<void*> data_tab_;

    // true if packet is received, false if it's is lost or repaired
    core::Array<bool> recv_tab_;

    // for debug logging
    core::Array<char> status_;

    bool has_new_packets_;
    bool decoding_finished_;

    size_t max_block_length_;

    status::StatusCode init_status_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_OPENFEC_DECODER_H_
