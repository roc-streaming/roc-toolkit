/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_config/config.h"
#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_fec/of_block_encoder.h"

namespace roc {
namespace fec {

namespace {

const size_t SYMB_SZ = ROC_CONFIG_DEFAULT_PACKET_SIZE;

} // namespace

OF_BlockEncoder::OF_BlockEncoder(   core::IByteBufferComposer& composer,
                                    fec_codec_type_t fec_type)
    : fec_type_(fec_type)
    , of_inst_(NULL)
    , composer_(composer)
    , sym_tab_(N_DATA_PACKETS + N_FEC_PACKETS)
    , buffers_(N_DATA_PACKETS + N_FEC_PACKETS) {

    // Use Reed-Solomon Codec.
    if (fec_type_ == OF_REED_SOLOMON_2_M) {
        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;

        roc_log(LOG_TRACE, "initializing Reed-Solomon encoder");

        fec_codec_params_.rs_params_.m = 8;

        of_inst_params_ = (of_parameters_t*)&fec_codec_params_.rs_params_;
    // Use LDPC-Staircase.
    } else if (fec_type_ == OF_LDPC_STAIRCASE) {
        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;

        roc_log(LOG_TRACE, "initializing LDPC encoder");

        fec_codec_params_.ldpc_params_.prng_seed = 1297501556;
        fec_codec_params_.ldpc_params_.N1 = 7;

        of_inst_params_ = (of_parameters_t*)&fec_codec_params_.ldpc_params_; 
    } else {
        roc_panic("OF_BlockEncoder: wrong FEC type is chosen.");
    }

    of_inst_params_->nb_source_symbols = N_DATA_PACKETS;
    of_inst_params_->nb_repair_symbols = N_FEC_PACKETS;
    of_inst_params_->encoding_symbol_length = SYMB_SZ;
    of_verbosity = 0;

    if (OF_STATUS_OK != of_create_codec_instance(
                            &of_inst_, codec_id_, OF_ENCODER, 0)) {
        roc_panic("OF_BlockEncoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_inst_ == NULL);

    if (OF_STATUS_OK != of_set_fec_parameters(of_inst_, of_inst_params_)) {
        roc_panic("OF_BlockEncoder: of_set_fec_parameters() failed");
    }
}

OF_BlockEncoder::~OF_BlockEncoder() {
    of_release_codec_instance(of_inst_);
}

void OF_BlockEncoder::write(size_t index, const core::IByteBufferConstSlice& buffer) {
    if (index >= N_DATA_PACKETS) {
        roc_panic("OF_BlockEncoder: can't write more than %lu data buffers",
                  (unsigned long)N_DATA_PACKETS);
    }

    if (!buffer) {
        roc_panic("OF_BlockEncoder: NULL buffer");
    }

    if ((uintptr_t)buffer.data() % 8 != 0) {
        roc_panic("OF_BlockEncoder: buffer data should be 8-byte aligned");
    }

    // const_cast<> is OK since OpenFEC will not modify this buffer.
    sym_tab_[index] = const_cast<uint8_t*>(buffer.data());
    buffers_[index] = buffer;
}

void OF_BlockEncoder::commit() {
    for (size_t i = 0; i < N_FEC_PACKETS; ++i) {
        if (core::IByteBufferPtr buffer = composer_.compose()) {
            buffer->set_size(SYMB_SZ);
            sym_tab_[N_DATA_PACKETS + i] = buffer->data();
            buffers_[N_DATA_PACKETS + i] = *buffer;
        } else {
            roc_log(LogDebug, "lOF_BlockEncoder can't allocate buffer");
            sym_tab_[N_DATA_PACKETS + i] = NULL;
        }
    }

    for (size_t i = N_DATA_PACKETS; i < N_DATA_PACKETS + N_FEC_PACKETS; ++i) {
        if (OF_STATUS_OK != of_build_repair_symbol(of_inst_, &sym_tab_[0], (uint32_t)i)) {
            roc_panic("OF_BlockEncoder: of_build_repair_symbol() failed");
        }
    }
}

core::IByteBufferConstSlice OF_BlockEncoder::read(size_t index) {
    if (index >= N_FEC_PACKETS) {
        roc_panic("OF_BlockEncoder: can't read more than %lu fec buffers",
                  (unsigned long)N_FEC_PACKETS);
    }

    return buffers_[N_DATA_PACKETS + index];
}

void OF_BlockEncoder::reset() {
    for (size_t i = 0; i < buffers_.size(); ++i) {
        sym_tab_[i] = NULL;
        buffers_[i] = core::IByteBufferConstSlice();
    }
}

} // namespace fec
} // namespace roc
