/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/openfec_encoder.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_packet/fec_scheme.h"

namespace roc {
namespace fec {

OpenfecEncoder::OpenfecEncoder(const CodecConfig& config,
                               packet::PacketFactory& packet_factory,
                               core::IArena& arena)
    : IBlockEncoder(arena)
    , sblen_(0)
    , rblen_(0)
    , payload_size_(0)
    , of_sess_(NULL)
    , buff_tab_(arena)
    , data_tab_(arena)
    , init_status_(status::NoStatus) {
    switch (config.scheme) {
#ifdef OF_USE_REED_SOLOMON_2_M_CODEC
    case packet::FEC_ReedSolomon_M8: {
        roc_log(LogDebug, "openfec encoder: initializing: codec=rs m=%u",
                (unsigned)config.rs_m);

        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
        codec_params_.rs_params_.m = config.rs_m;

        of_sess_params_ = (of_parameters_t*)&codec_params_.rs_params_;

        max_block_length_ = size_t(1 << config.rs_m) - 1;
    } break;
#endif // OF_USE_REED_SOLOMON_2_M_CODEC

#ifdef OF_USE_LDPC_STAIRCASE_CODEC
    case packet::FEC_LDPC_Staircase: {
        roc_log(LogDebug, "openfec encoder: initializing: codec=ldpc prng_seed=%ld n1=%d",
                (long)config.ldpc_prng_seed, (int)config.ldpc_N1);

        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;
        codec_params_.ldpc_params_.prng_seed = config.ldpc_prng_seed;
        codec_params_.ldpc_params_.N1 = config.ldpc_N1;

        of_sess_params_ = (of_parameters_t*)&codec_params_.ldpc_params_;

        max_block_length_ = OF_LDPC_STAIRCASE_MAX_NB_ENCODING_SYMBOLS_DEFAULT;
    } break;
#endif // OF_USE_LDPC_STAIRCASE_CODEC

    default:
        roc_log(LogError, "openfec encoder: unsupported fec scheme: scheme=%s",
                packet::fec_scheme_to_str(config.scheme));
        init_status_ = status::StatusBadConfig;
        return;
    }

    of_verbosity = 0;
    init_status_ = status::StatusOK;
}

OpenfecEncoder::~OpenfecEncoder() {
    if (of_sess_) {
        of_release_codec_instance(of_sess_);
    }
}

status::StatusCode OpenfecEncoder::init_status() const {
    return init_status_;
}

size_t OpenfecEncoder::max_block_length() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return max_block_length_;
}

size_t OpenfecEncoder::buffer_alignment() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return Alignment;
}

status::StatusCode
OpenfecEncoder::begin_block(size_t sblen, size_t rblen, size_t payload_size) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (sblen_ == sblen && rblen_ == rblen && payload_size_ == payload_size) {
        return status::StatusOK;
    }

    if (!resize_tabs_(sblen + rblen)) {
        roc_log(
            LogError,
            "openfec encoder: failed to resize tabs in begin_block, sblen=%lu, rblen=%lu",
            (unsigned long)sblen, (unsigned long)rblen);
        return status::StatusNoMem;
    }

    sblen_ = sblen;
    rblen_ = rblen;
    payload_size_ = payload_size;

    update_session_params_(sblen, rblen, payload_size);
    reset_session_();

    return status::StatusOK;
}

void OpenfecEncoder::set_buffer(size_t index, const core::Slice<uint8_t>& buffer) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (index >= sblen_ + rblen_) {
        roc_panic("openfec encoder: can't write more than %lu data buffers",
                  (unsigned long)sblen_);
    }

    if (!buffer) {
        roc_panic("openfec encoder: null buffer");
    }

    if (buffer.size() == 0 || buffer.size() != payload_size_) {
        roc_panic("openfec encoder: invalid payload size: cur=%lu new=%lu",
                  (unsigned long)payload_size_, (unsigned long)buffer.size());
    }

    if ((uintptr_t)buffer.data() % Alignment != 0) {
        roc_panic("openfec encoder: buffer data should be %d-byte aligned: index=%lu",
                  (int)Alignment, (unsigned long)index);
    }

    data_tab_[index] = buffer.data();
    buff_tab_[index] = buffer;
}

void OpenfecEncoder::fill_buffers() {
    roc_panic_if(init_status_ != status::StatusOK);

    for (size_t i = sblen_; i < sblen_ + rblen_; ++i) {
        roc_log(LogTrace, "openfec encoder: of_build_repair_symbol(): index=%lu",
                (unsigned long)i);

        if (OF_STATUS_OK
            != of_build_repair_symbol(of_sess_, &data_tab_[0], (uint32_t)i)) {
            roc_panic("openfec encoder: of_build_repair_symbol() failed");
        }
    }
}

void OpenfecEncoder::end_block() {
    roc_panic_if(init_status_ != status::StatusOK);

    for (size_t i = 0; i < buff_tab_.size(); ++i) {
        data_tab_[i] = NULL;
        buff_tab_[i] = core::Slice<uint8_t>();
    }
}

bool OpenfecEncoder::resize_tabs_(size_t size) {
    if (!buff_tab_.resize(size)) {
        return false;
    }

    if (!data_tab_.resize(size)) {
        return false;
    }

    return true;
}

void OpenfecEncoder::update_session_params_(size_t sblen,
                                            size_t rblen,
                                            size_t payload_size) {
    of_sess_params_->nb_source_symbols = (uint32_t)sblen;
    of_sess_params_->nb_repair_symbols = (uint32_t)rblen;
    of_sess_params_->encoding_symbol_length = (uint32_t)payload_size;
}

void OpenfecEncoder::reset_session_() {
    if (of_sess_ != NULL) {
        roc_log(LogTrace, "openfec encoder: of_release_codec_instance()");

        of_release_codec_instance(of_sess_);
        of_sess_ = NULL;
    }

    roc_log(LogTrace, "openfec encoder: of_create_codec_instance()");

    if (OF_STATUS_OK != of_create_codec_instance(&of_sess_, codec_id_, OF_ENCODER, 0)) {
        roc_panic("openfec encoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_sess_ == NULL);

    roc_log(
        LogTrace,
        "openfec encoder: of_set_fec_parameters(): nb_src=%lu nb_rpr=%lu symbol_len=%lu",
        (unsigned long)of_sess_params_->nb_source_symbols,
        (unsigned long)of_sess_params_->nb_repair_symbols,
        (unsigned long)of_sess_params_->encoding_symbol_length);

    if (OF_STATUS_OK != of_set_fec_parameters(of_sess_, of_sess_params_)) {
        roc_panic("openfec encoder: of_set_fec_parameters() failed");
    }
}

} // namespace fec
} // namespace roc
