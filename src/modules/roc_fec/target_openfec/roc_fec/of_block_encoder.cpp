/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/of_block_encoder.h"
#include "roc_config/config.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace fec {

namespace {

const size_t SYMB_SZ = ROC_CONFIG_DEFAULT_PACKET_SIZE;

} // namespace

OFBlockEncoder::OFBlockEncoder(const Config& config, core::IByteBufferComposer& composer)
    : blk_source_packets_(config.n_source_packets)
    , blk_repair_packets_(config.n_repair_packets)
    , of_sess_(NULL)
    , composer_(composer)
    , buff_tab_(config.n_source_packets + config.n_repair_packets)
    , data_tab_(config.n_source_packets + config.n_repair_packets) {
    if (config.codec == ReedSolomon2m) {
        roc_log(LogDebug, "initializing Reed-Solomon encoder");

        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
        codec_params_.rs_params_.m = config.rs_m;

        of_sess_params_ = (of_parameters_t*)&codec_params_.rs_params_;
    } else if (config.codec == LDPCStaircase) {
        roc_log(LogDebug, "initializing LDPC encoder");

        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;
        codec_params_.ldpc_params_.prng_seed = config.ldpc_prng_seed;
        codec_params_.ldpc_params_.N1 = config.ldpc_N1;

        of_sess_params_ = (of_parameters_t*)&codec_params_.ldpc_params_;
    } else {
        roc_panic("block encoder: wrong FEC type is chosen.");
    }

    of_sess_params_->nb_source_symbols = (uint32_t)blk_source_packets_;
    of_sess_params_->nb_repair_symbols = (uint32_t)blk_repair_packets_;
    of_sess_params_->encoding_symbol_length = SYMB_SZ;
    of_verbosity = 0;

    if (OF_STATUS_OK != of_create_codec_instance(&of_sess_, codec_id_, OF_ENCODER, 0)) {
        roc_panic("block encoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_sess_ == NULL);

    if (OF_STATUS_OK != of_set_fec_parameters(of_sess_, of_sess_params_)) {
        roc_panic("block encoder: of_set_fec_parameters() failed");
    }
}

OFBlockEncoder::~OFBlockEncoder() {
    of_release_codec_instance(of_sess_);
}

void OFBlockEncoder::write(size_t index, const core::IByteBufferConstSlice& buffer) {
    if (index >= blk_source_packets_) {
        roc_panic("block encoder: can't write more than %lu data buffers",
                  (unsigned long)blk_source_packets_);
    }

    if (!buffer) {
        roc_panic("block encoder: NULL buffer");
    }

    if ((uintptr_t)buffer.data() % 8 != 0) {
        roc_panic("block encoder: buffer data should be 8-byte aligned");
    }

    // const_cast<> is OK since OpenFEC will not modify this buffer.
    data_tab_[index] = const_cast<uint8_t*>(buffer.data());
    buff_tab_[index] = buffer;
}

void OFBlockEncoder::commit() {
    for (size_t i = 0; i < blk_repair_packets_; ++i) {
        if (core::IByteBufferPtr buffer = composer_.compose()) {
            buffer->set_size(SYMB_SZ);
            data_tab_[blk_source_packets_ + i] = buffer->data();
            buff_tab_[blk_source_packets_ + i] = *buffer;
        } else {
            roc_log(LogDebug, "OFBlockEncoder can't allocate buffer");
            data_tab_[blk_source_packets_ + i] = NULL;
        }
    }

    for (size_t i = blk_source_packets_; i < blk_source_packets_ + blk_repair_packets_;
         ++i) {
        if (OF_STATUS_OK
            != of_build_repair_symbol(of_sess_, &data_tab_[0], (uint32_t)i)) {
            roc_panic("block encoder: of_build_repair_symbol() failed");
        }
    }
}

core::IByteBufferConstSlice OFBlockEncoder::read(size_t index) {
    if (index >= blk_repair_packets_) {
        roc_panic("block encoder: can't read more than %lu fec buffers",
                  (unsigned long)blk_repair_packets_);
    }

    return buff_tab_[blk_source_packets_ + index];
}

void OFBlockEncoder::reset() {
    for (size_t i = 0; i < buff_tab_.size(); ++i) {
        data_tab_[i] = NULL;
        buff_tab_[i] = core::IByteBufferConstSlice();
    }
}

size_t OFBlockEncoder::n_data_packets() const {
    return blk_source_packets_;
}

size_t OFBlockEncoder::n_fec_packets() const {
    return blk_repair_packets_;
}

} // namespace fec
} // namespace roc
