/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/of_encoder.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace fec {

OFEncoder::OFEncoder(const Config& config,
                     size_t payload_size,
                     core::IAllocator& allocator)
    : blk_source_packets_(config.n_source_packets)
    , blk_repair_packets_(config.n_repair_packets)
    , of_sess_(NULL)
    , buff_tab_(allocator, config.n_source_packets + config.n_repair_packets)
    , data_tab_(allocator, config.n_source_packets + config.n_repair_packets) {
    buff_tab_.resize(buff_tab_.max_size());
    data_tab_.resize(data_tab_.max_size());
    if (config.codec == ReedSolomon8m) {
        roc_log(LogDebug, "of encoder: initializing Reed-Solomon encoder");

        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
        codec_params_.rs_params_.m = config.rs_m;

        of_sess_params_ = (of_parameters_t*)&codec_params_.rs_params_;
    } else if (config.codec == LDPCStaircase) {
        roc_log(LogDebug, "of encoder: initializing LDPC encoder");

        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;
        codec_params_.ldpc_params_.prng_seed = config.ldpc_prng_seed;
        codec_params_.ldpc_params_.N1 = config.ldpc_N1;

        of_sess_params_ = (of_parameters_t*)&codec_params_.ldpc_params_;
    } else {
        roc_panic("of encoder: wrong FEC type is chosen.");
    }

    of_sess_params_->nb_source_symbols = (uint32_t)blk_source_packets_;
    of_sess_params_->nb_repair_symbols = (uint32_t)blk_repair_packets_;
    of_sess_params_->encoding_symbol_length = (uint32_t)payload_size;
    of_verbosity = 0;

    if (OF_STATUS_OK != of_create_codec_instance(&of_sess_, codec_id_, OF_ENCODER, 0)) {
        roc_panic("of encoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_sess_ == NULL);

    if (OF_STATUS_OK != of_set_fec_parameters(of_sess_, of_sess_params_)) {
        roc_panic("of encoder: of_set_fec_parameters() failed");
    }
}

OFEncoder::~OFEncoder() {
    of_release_codec_instance(of_sess_);
}

size_t OFEncoder::alignment() const {
    return Alignment;
}

void OFEncoder::set(size_t index, const core::Slice<uint8_t>& buffer) {
    if (index >= blk_source_packets_ + blk_repair_packets_) {
        roc_panic("of encoder: can't write more than %lu data buffers",
                  (unsigned long)blk_source_packets_);
    }

    if (!buffer) {
        roc_panic("of encoder: null buffer");
    }

    if ((uintptr_t)buffer.data() % Alignment != 0) {
        roc_panic("of encoder: buffer data should be %d-byte aligned: index=%lu",
                  (int)Alignment, (unsigned long)index);
    }

    data_tab_[index] = buffer.data();
    buff_tab_[index] = buffer;
}

void OFEncoder::commit() {
    for (size_t i = blk_source_packets_; i < blk_source_packets_ + blk_repair_packets_;
         ++i) {
        if (OF_STATUS_OK
            != of_build_repair_symbol(of_sess_, &data_tab_[0], (uint32_t)i)) {
            roc_panic("of encoder: of_build_repair_symbol() failed");
        }
    }
}

void OFEncoder::reset() {
    for (size_t i = 0; i < buff_tab_.size(); ++i) {
        data_tab_[i] = NULL;
        buff_tab_[i] = core::Slice<uint8_t>();
    }
}

} // namespace fec
} // namespace roc
