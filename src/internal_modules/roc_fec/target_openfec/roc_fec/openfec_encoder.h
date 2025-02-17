/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/target_openfec/roc_fec/openfec_encoder.h
//! @brief Encoder implementation using OpenFEC library.

#ifndef ROC_FEC_OPENFEC_ENCODER_H_
#define ROC_FEC_OPENFEC_ENCODER_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_fec/codec_config.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/units.h"

extern "C" {
#include <of_openfec_api.h>
}

namespace roc {
namespace fec {

//! Encoder implementation using OpenFEC library.
class OpenfecEncoder : public IBlockEncoder, public core::NonCopyable<> {
public:
    //! Initialize.
    OpenfecEncoder(const CodecConfig& config,
                   packet::PacketFactory& packet_factory,
                   core::IArena& arena);

    virtual ~OpenfecEncoder();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Get the maximum number of encoding symbols for the scheme being used.
    virtual size_t max_block_length() const;

    //! Get buffer alignment requirement.
    virtual size_t buffer_alignment() const;

    //! Start block.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    begin_block(size_t sblen, size_t rblen, size_t payload_size);

    //! Store packet data for current block.
    virtual void set_buffer(size_t index, const core::Slice<uint8_t>& buffer);

    //! Fill repair packets.
    virtual void fill_buffers();

    //! Finish block.
    virtual void end_block();

private:
    bool resize_tabs_(size_t size);
    void reset_session_();
    void update_session_params_(size_t sblen, size_t rblen, size_t payload_size);

    enum { Alignment = 8 };

    size_t sblen_;
    size_t rblen_;

    size_t payload_size_;

    of_session_t* of_sess_;
    of_parameters_t* of_sess_params_;

    of_codec_id_t codec_id_;
    union {
#ifdef OF_USE_REED_SOLOMON_2_M_CODEC
        of_rs_2_m_parameters_t rs_params_;
#endif
#ifdef OF_USE_LDPC_STAIRCASE_CODEC
        of_ldpc_parameters ldpc_params_;
#endif
    } codec_params_;

    core::Array<core::Slice<uint8_t> > buff_tab_;
    core::Array<void*> data_tab_;

    size_t max_block_length_;

    status::StatusCode init_status_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_OPENFEC_ENCODER_H_
