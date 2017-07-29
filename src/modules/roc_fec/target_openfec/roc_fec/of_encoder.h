/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/target_openfec/roc_fec/of_encoder.h
//! @brief Encoder implementation using OpenFEC library.

#ifndef ROC_FEC_OF_ENCODER_H_
#define ROC_FEC_OF_ENCODER_H_

#include "roc_core/array.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_fec/config.h"
#include "roc_fec/iencoder.h"
#include "roc_packet/units.h"

extern "C" {
#include <of_openfec_api.h>
}

#ifndef OF_USE_ENCODER
#error "OF_USE_ENCODER undefined"
#endif

#ifndef OF_USE_LDPC_STAIRCASE_CODEC
#error "OF_USE_LDPC_STAIRCASE_CODEC undefined"
#endif

namespace roc {
namespace fec {

//! Encoder implementation using OpenFEC library.
class OFEncoder : public IEncoder, public core::NonCopyable<> {
public:
    //! Initialize.
    explicit OFEncoder(const Config& config,
                       size_t payload_size,
                       core::IAllocator& allocator);

    virtual ~OFEncoder();

    //! Get buffer alignment requirement.
    virtual size_t alignment() const;

    //! Store packet data for current block.
    virtual void set(size_t index, const core::Slice<uint8_t>& buffer);

    //! Fill repair packets.
    virtual void commit();

    //! Reset current block.
    virtual void reset();

private:
    enum { Alignment = 8 };

    const size_t blk_source_packets_;
    const size_t blk_repair_packets_;

    of_session_t* of_sess_;
    of_parameters_t* of_sess_params_;

    of_codec_id_t codec_id_;
    union {
        of_ldpc_parameters ldpc_params_;
        of_rs_2_m_parameters_t rs_params_;
    } codec_params_;

    core::Array<core::Slice<uint8_t> > buff_tab_;
    core::Array<void*> data_tab_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_OF_ENCODER_H_
