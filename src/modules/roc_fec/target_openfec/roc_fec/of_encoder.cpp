/*
 * Copyright (c) 2015 Roc authors
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

OFEncoder::OFEncoder(const CodecConfig& config,
                     core::BufferPool<uint8_t>&,
                     core::IAllocator& allocator)
    : sblen_(0)
    , rblen_(0)
    , payload_size_(0)
    , of_sess_(NULL)
    , buff_tab_(allocator)
    , data_tab_(allocator)
    , valid_(false) {
    if (config.scheme == packet::FEC_ReedSolomon_M8) {
        roc_log(LogDebug, "of encoder: initializing: codec=rs m=%u",
                (unsigned)config.rs_m);

        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
        codec_params_.rs_params_.m = config.rs_m;

        of_sess_params_ = (of_parameters_t*)&codec_params_.rs_params_;

        max_block_length_ = OF_REED_SOLOMON_MAX_NB_ENCODING_SYMBOLS_DEFAULT;
    } else if (config.scheme == packet::FEC_LDPC_Staircase) {
        roc_log(LogDebug, "of encoder: initializing: codec=ldpc prng_seed=%ld n1=%d",
                (long)config.ldpc_prng_seed, (int)config.ldpc_N1);

        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;
        codec_params_.ldpc_params_.prng_seed = config.ldpc_prng_seed;
        codec_params_.ldpc_params_.N1 = config.ldpc_N1;

        of_sess_params_ = (of_parameters_t*)&codec_params_.ldpc_params_;

        max_block_length_ = OF_LDPC_STAIRCASE_MAX_NB_ENCODING_SYMBOLS_DEFAULT;
    } else {
        roc_panic("of encoder: unexpected fec scheme");
    }

    of_verbosity = 0;

    valid_ = true;
}

OFEncoder::~OFEncoder() {
    if (of_sess_) {
        of_release_codec_instance(of_sess_);
    }
}

bool OFEncoder::valid() const {
    return valid_;
}

size_t OFEncoder::alignment() const {
    return Alignment;
}

size_t OFEncoder::max_block_length() const {
    roc_panic_if_not(valid());

    return max_block_length_;
}

bool OFEncoder::begin(size_t sblen, size_t rblen, size_t payload_size) {
    roc_panic_if_not(valid());

    if (sblen_ == sblen && rblen_ == rblen && payload_size_ == payload_size) {
        return true;
    }

    if (!resize_tabs_(sblen + rblen)) {
        return false;
    }

    sblen_ = sblen;
    rblen_ = rblen;
    payload_size_ = payload_size;

    update_session_params_(sblen, rblen, payload_size);
    reset_session_();

    return true;
}

void OFEncoder::set(size_t index, const core::Slice<uint8_t>& buffer) {
    roc_panic_if_not(valid());

    if (index >= sblen_ + rblen_) {
        roc_panic("of encoder: can't write more than %lu data buffers",
                  (unsigned long)sblen_);
    }

    if (!buffer) {
        roc_panic("of encoder: null buffer");
    }

    if (buffer.size() == 0 || buffer.size() != payload_size_) {
        roc_panic("of encoder: invalid payload size: cur=%lu new=%lu",
                  (unsigned long)payload_size_, (unsigned long)buffer.size());
    }

    if ((uintptr_t)buffer.data() % Alignment != 0) {
        roc_panic("of encoder: buffer data should be %d-byte aligned: index=%lu",
                  (int)Alignment, (unsigned long)index);
    }

    data_tab_[index] = buffer.data();
    buff_tab_[index] = buffer;
}

void OFEncoder::fill() {
    roc_panic_if_not(valid());

    for (size_t i = sblen_; i < sblen_ + rblen_; ++i) {
        roc_log(LogTrace, "of encoder: of_build_repair_symbol(): index=%lu",
                (unsigned long)i);

        if (OF_STATUS_OK
            != of_build_repair_symbol(of_sess_, &data_tab_[0], (uint32_t)i)) {
            roc_panic("of encoder: of_build_repair_symbol() failed");
        }
    }
}

void OFEncoder::end() {
    roc_panic_if_not(valid());

    for (size_t i = 0; i < buff_tab_.size(); ++i) {
        data_tab_[i] = NULL;
        buff_tab_[i] = core::Slice<uint8_t>();
    }
}

bool OFEncoder::resize_tabs_(size_t size) {
    if (!buff_tab_.resize(size)) {
        return false;
    }

    if (!data_tab_.resize(size)) {
        return false;
    }

    return true;
}

void OFEncoder::update_session_params_(size_t sblen, size_t rblen, size_t payload_size) {
    of_sess_params_->nb_source_symbols = (uint32_t)sblen;
    of_sess_params_->nb_repair_symbols = (uint32_t)rblen;
    of_sess_params_->encoding_symbol_length = (uint32_t)payload_size;
}

void OFEncoder::reset_session_() {
    if (of_sess_ != NULL) {
        roc_log(LogTrace, "of encoder: of_release_codec_instance()");

        of_release_codec_instance(of_sess_);
        of_sess_ = NULL;
    }

    roc_log(LogTrace, "of encoder: of_create_codec_instance()");

    if (OF_STATUS_OK != of_create_codec_instance(&of_sess_, codec_id_, OF_ENCODER, 0)) {
        roc_panic("of encoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_sess_ == NULL);

    roc_log(LogTrace,
            "of encoder: of_set_fec_parameters(): nb_src=%lu nb_rpr=%lu symbol_len=%lu",
            (unsigned long)of_sess_params_->nb_source_symbols,
            (unsigned long)of_sess_params_->nb_repair_symbols,
            (unsigned long)of_sess_params_->encoding_symbol_length);

    if (OF_STATUS_OK != of_set_fec_parameters(of_sess_, of_sess_params_)) {
        roc_panic("of encoder: of_set_fec_parameters() failed");
    }
}

} // namespace fec
} // namespace roc
