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
#include "roc_core/stddefs.h"
#include "roc_fec/of_block_encoder.h"

namespace roc {
namespace fec {

namespace {

const size_t SYMB_SZ = ROC_CONFIG_DEFAULT_PACKET_SIZE;

} // namespace

OFBlockEncoder::OFBlockEncoder(const Config &config,
                                core::IByteBufferComposer& composer)
    : n_data_packets_(config.n_source_packets)
    , n_fec_packets_(config.n_repair_packets)
    , of_inst_(NULL)
    , composer_(composer)
    , sym_tab_(config.n_source_packets + config.n_repair_packets)
    , buffers_(config.n_source_packets + config.n_repair_packets) {
    if (config.codec == ReedSolomon2m) {
        // Use Reed-Solomon Codec.
        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;

        roc_log(LogDebug, "initializing Reed-Solomon encoder");

        fec_codec_params_.rs_params_.m = config.rs_m;

        of_inst_params_ = (of_parameters_t*)&fec_codec_params_.rs_params_;
    } else if (config.codec == LDPCStaircase) {
        // Use LDPC-Staircase.
        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;

        roc_log(LogDebug, "initializing LDPC encoder");

        fec_codec_params_.ldpc_params_.prng_seed = config.ldpc_prng_seed;
        fec_codec_params_.ldpc_params_.N1 = config.ldpc_N1;

        of_inst_params_ = (of_parameters_t*)&fec_codec_params_.ldpc_params_;
    } else {
        roc_panic("OFBlockEncoder: wrong FEC type is chosen.");
    }

    of_inst_params_->nb_source_symbols = (uint32_t)n_data_packets_;
    of_inst_params_->nb_repair_symbols = (uint32_t)n_fec_packets_;
    of_inst_params_->encoding_symbol_length = SYMB_SZ;
    of_verbosity = 0;

    if (OF_STATUS_OK != of_create_codec_instance(&of_inst_, codec_id_, OF_ENCODER, 0)) {
        roc_panic("OFBlockEncoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_inst_ == NULL);

    if (OF_STATUS_OK != of_set_fec_parameters(of_inst_, of_inst_params_)) {
        roc_panic("OFBlockEncoder: of_set_fec_parameters() failed");
    }
}

OFBlockEncoder::~OFBlockEncoder() {
    of_release_codec_instance(of_inst_);
}

void OFBlockEncoder::write(size_t index, const core::IByteBufferConstSlice& buffer) {
    if (index >= n_data_packets_) {
        roc_panic("OFBlockEncoder: can't write more than %lu data buffers",
                  (unsigned long)n_data_packets_);
    }

    if (!buffer) {
        roc_panic("OFBlockEncoder: NULL buffer");
    }

    if ((uintptr_t)buffer.data() % 8 != 0) {
        roc_panic("OFBlockEncoder: buffer data should be 8-byte aligned");
    }

    // const_cast<> is OK since OpenFEC will not modify this buffer.
    sym_tab_[index] = const_cast<uint8_t*>(buffer.data());
    buffers_[index] = buffer;
}

void OFBlockEncoder::commit() {
    for (size_t i = 0; i < n_fec_packets_; ++i) {
        if (core::IByteBufferPtr buffer = composer_.compose()) {
            buffer->set_size(SYMB_SZ);
            sym_tab_[n_data_packets_ + i] = buffer->data();
            buffers_[n_data_packets_ + i] = *buffer;
        } else {
            roc_log(LogDebug, "OFBlockEncoder can't allocate buffer");
            sym_tab_[n_data_packets_ + i] = NULL;
        }
    }

    for (size_t i = n_data_packets_; i < n_data_packets_ + n_fec_packets_; ++i) {
        if (OF_STATUS_OK != of_build_repair_symbol(of_inst_, &sym_tab_[0], (uint32_t)i)) {
            roc_panic("OFBlockEncoder: of_build_repair_symbol() failed");
        }
    }
}

core::IByteBufferConstSlice OFBlockEncoder::read(size_t index) {
    if (index >= n_fec_packets_) {
        roc_panic("OFBlockEncoder: can't read more than %lu fec buffers",
                  (unsigned long)n_fec_packets_);
    }

    return buffers_[n_data_packets_ + index];
}

void OFBlockEncoder::reset() {
    for (size_t i = 0; i < buffers_.size(); ++i) {
        sym_tab_[i] = NULL;
        buffers_[i] = core::IByteBufferConstSlice();
    }
}

size_t OFBlockEncoder::n_data_packets() const {
    return n_data_packets_;
}

size_t OFBlockEncoder::n_fec_packets() const {
    return n_fec_packets_;
}

} // namespace fec
} // namespace roc
