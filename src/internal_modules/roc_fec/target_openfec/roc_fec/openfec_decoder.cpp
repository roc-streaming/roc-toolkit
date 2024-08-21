/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/openfec_decoder.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_packet/fec_scheme.h"

extern "C" {
#include <of_mem.h>
}

namespace roc {
namespace fec {

OpenfecDecoder::OpenfecDecoder(const CodecConfig& config,
                               packet::PacketFactory& packet_factory,
                               core::IArena& arena)
    : IBlockDecoder(arena)
    , sblen_(0)
    , rblen_(0)
    , payload_size_(0)
    , max_index_(0)
    , of_sess_(NULL)
    , of_sess_params_(NULL)
    , packet_factory_(packet_factory)
    , buff_tab_(arena)
    , data_tab_(arena)
    , recv_tab_(arena)
    , status_(arena)
    , has_new_packets_(false)
    , decoding_finished_(false)
    , init_status_(status::NoStatus) {
    switch (config.scheme) {
#ifdef OF_USE_REED_SOLOMON_2_M_CODEC
    case packet::FEC_ReedSolomon_M8: {
        roc_log(LogDebug, "openfec decoder: initializing: codec=rs m=%u",
                (unsigned)config.rs_m);

        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
        codec_params_.rs_params_.m = config.rs_m;

        of_sess_params_ = (of_parameters_t*)&codec_params_.rs_params_;

        max_block_length_ = size_t(1 << config.rs_m) - 1;
    } break;
#endif // OF_USE_REED_SOLOMON_2_M_CODEC

#ifdef OF_USE_LDPC_STAIRCASE_CODEC
    case packet::FEC_LDPC_Staircase: {
        roc_log(LogDebug, "openfec decoder: initializing: codec=ldpc prng_seed=%ld n1=%d",
                (long)config.ldpc_prng_seed, (int)config.ldpc_N1);

        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;
        codec_params_.ldpc_params_.prng_seed = config.ldpc_prng_seed;
        codec_params_.ldpc_params_.N1 = config.ldpc_N1;

        of_sess_params_ = (of_parameters_t*)&codec_params_.ldpc_params_;

        max_block_length_ = OF_LDPC_STAIRCASE_MAX_NB_ENCODING_SYMBOLS_DEFAULT;
    } break;
#endif // OF_USE_LDPC_STAIRCASE_CODEC

    default:
        roc_log(LogError, "openfec decoder: unsupported fec scheme: scheme=%s",
                packet::fec_scheme_to_str(config.scheme));
        init_status_ = status::StatusBadConfig;
        return;
    }

    of_verbosity = 0;
    init_status_ = status::StatusOK;
}

OpenfecDecoder::~OpenfecDecoder() {
    if (of_sess_) {
        destroy_session_();
    }
}

status::StatusCode OpenfecDecoder::init_status() const {
    return init_status_;
}

size_t OpenfecDecoder::max_block_length() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return max_block_length_;
}

status::StatusCode
OpenfecDecoder::begin_block(size_t sblen, size_t rblen, size_t payload_size) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!resize_tabs_(sblen + rblen)) {
        roc_log(
            LogError,
            "openfec decoder: failed to resize tabs in begin_block, sblen=%lu, rblen=%lu",
            (unsigned long)sblen, (unsigned long)rblen);
        return status::StatusNoMem;
    }

    sblen_ = sblen;
    rblen_ = rblen;
    payload_size_ = payload_size;
    max_index_ = 0;

    update_session_params_(sblen, rblen, payload_size);
    reset_session_();

    return status::StatusOK;
}

void OpenfecDecoder::set_buffer(size_t index, const core::Slice<uint8_t>& buffer) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (index >= sblen_ + rblen_) {
        roc_panic("openfec decoder: index out of bounds: index=%lu size=%lu",
                  (unsigned long)index, (unsigned long)(sblen_ + rblen_));
    }

    if (!buffer) {
        roc_panic("openfec decoder: null buffer");
    }

    if (buffer.size() == 0 || buffer.size() != payload_size_) {
        roc_panic("openfec decoder: invalid payload size: cur=%lu new=%lu",
                  (unsigned long)payload_size_, (unsigned long)buffer.size());
    }

    if (buff_tab_[index]) {
        roc_panic("openfec decoder: can't overwrite buffer: index=%lu",
                  (unsigned long)index);
    }

    has_new_packets_ = true;

    buff_tab_[index] = buffer;
    data_tab_[index] = buffer.data();
    recv_tab_[index] = true;

    // register new packet and try to repair more packets
    roc_log(LogTrace, "openfec decoder: of_decode_with_new_symbol(): index=%lu",
            (unsigned long)index);

    if (of_decode_with_new_symbol(of_sess_, data_tab_[index], (unsigned int)index)
        != OF_STATUS_OK) {
        roc_panic("openfec decoder: can't add packet to OF session");
    }

    if (max_index_ < index) {
        max_index_ = index;
    }
}

core::Slice<uint8_t> OpenfecDecoder::repair_buffer(size_t index) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!buff_tab_[index]) {
        update_();
        fix_buffer_(index);
    }

    return buff_tab_[index];
}

void OpenfecDecoder::end_block() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (of_sess_ != NULL) {
        report_();
        destroy_session_();
    }

    reset_tabs_();

    has_new_packets_ = false;
    decoding_finished_ = false;
}

void OpenfecDecoder::update_session_params_(size_t sblen,
                                            size_t rblen,
                                            size_t payload_size) {
    of_sess_params_->nb_source_symbols = (uint32_t)sblen;
    of_sess_params_->nb_repair_symbols = (uint32_t)rblen;
    of_sess_params_->encoding_symbol_length = (uint32_t)payload_size;
}

void OpenfecDecoder::reset_tabs_() {
    for (size_t i = 0; i < buff_tab_.size(); ++i) {
        buff_tab_[i] = core::Slice<uint8_t>();
        data_tab_[i] = NULL;
        recv_tab_[i] = false;
    }
}

bool OpenfecDecoder::resize_tabs_(size_t size) {
    if (!buff_tab_.resize(size)) {
        return false;
    }
    if (!data_tab_.resize(size)) {
        return false;
    }
    if (!recv_tab_.resize(size)) {
        return false;
    }
    if (!status_.resize(size + 2)) {
        return false;
    }

    return true;
}

void OpenfecDecoder::update_() {
    roc_panic_if(of_sess_ == NULL);

    if (!has_new_packets_) {
        return;
    }

    decode_();

    roc_log(LogTrace, "openfec decoder: of_get_source_symbols_tab()");

    of_get_source_symbols_tab(of_sess_, &data_tab_[0]);

    has_new_packets_ = false;
}

void OpenfecDecoder::decode_() {
    if (decoding_finished_ && is_optimal_()) {
        return;
    }

    if (!has_n_packets_(sblen_)) {
        return;
    }

    if (decoding_finished_) {
        // it's not allowed to decode twice, so we recreate the session
        reset_session_();

        roc_log(LogTrace, "openfec decoder: of_set_available_symbols()");

        if (of_set_available_symbols(of_sess_, &data_tab_[0]) != OF_STATUS_OK) {
            roc_panic("openfec decoder: can't add packets to OF session");
        }
    }

    // try to repair more packets
    roc_log(LogTrace, "openfec decoder: of_finish_decoding()");

    if (of_finish_decoding(of_sess_) != OF_STATUS_OK) {
        roc_log(LogTrace, "openfec decoder: of_finish_decoding() returned error");
        return;
    }

    decoding_finished_ = true;
}

// note: we have to calculate this every time because OpenFEC
// doesn't always report to us when it repairs a packet
bool OpenfecDecoder::has_n_packets_(size_t n_packets) const {
    size_t n = 0;
    for (size_t i = 0; i < data_tab_.size(); i++) {
        if (data_tab_[i]) {
            if (++n >= n_packets) {
                return true;
            }
        }
    }
    return false;
}

// returns true if the codec requires exactly k packets
// (number of source packets in block) to repair any
// source packet
//
// non-optimal codecs may require more packets, and the
// exact amount may be different every block
bool OpenfecDecoder::is_optimal_() const {
    return codec_id_ == OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
}

void OpenfecDecoder::reset_session_() {
    if (of_sess_ != NULL) {
        of_release_codec_instance(of_sess_);
        of_sess_ = NULL;
    }

    roc_log(LogTrace, "openfec decoder: of_create_codec_instance()");

    if (OF_STATUS_OK != of_create_codec_instance(&of_sess_, codec_id_, OF_DECODER, 0)) {
        roc_panic("openfec decoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_sess_ == NULL);

    roc_log(
        LogTrace,
        "openfec decoder: of_set_fec_parameters(): nb_src=%lu nb_rpr=%lu symbol_len=%lu",
        (unsigned long)of_sess_params_->nb_source_symbols,
        (unsigned long)of_sess_params_->nb_repair_symbols,
        (unsigned long)of_sess_params_->encoding_symbol_length);

    if (OF_STATUS_OK != of_set_fec_parameters(of_sess_, of_sess_params_)) {
        roc_panic("openfec decoder: of_set_fec_parameters() failed");
    }

    roc_log(LogTrace, "openfec decoder: of_set_callback_functions()");

    if (OF_STATUS_OK
        != of_set_callback_functions(
            of_sess_, source_cb_,
            // OpenFEC doesn't repair fec-packets in case of Reed-Solomon FEC
            // and prints curses to the console if we give it the callback for that
            codec_id_ == OF_CODEC_REED_SOLOMON_GF_2_M_STABLE ? NULL : repair_cb_,
            (void*)this)) {
        roc_panic("openfec decoder: of_set_callback_functions() failed");
    }
}

void OpenfecDecoder::destroy_session_() {
    roc_log(LogTrace, "openfec decoder: of_release_codec_instance()");

    of_release_codec_instance(of_sess_);
    of_sess_ = NULL;

    // OpenFEC may allocate memory without calling source_cb_()
    // we should free() such memory manually
    for (size_t i = 0; i < sblen_; i++) {
        if (data_tab_[i] == NULL) {
            continue;
        }
        if (buff_tab_[i] && buff_tab_[i].data() == data_tab_[i]) {
            continue;
        }

        roc_log(LogTrace, "openfec decoder: of_free(): index=%lu", (unsigned long)i);
        of_free(data_tab_[i]);

        data_tab_[i] = NULL;
    }
}

void OpenfecDecoder::report_() {
    size_t n_lost = 0, n_repaired = 0;

    size_t tab_size = max_index_;
    if (tab_size < sblen_) {
        tab_size = sblen_;
    }

    status_[sblen_] = ' ';
    status_[tab_size] = '\0';

    for (size_t i = 0; i < tab_size; ++i) {
        char* status = (i < sblen_ ? &status_[i] : &status_[i + 1]);

        if (buff_tab_[i] || data_tab_[i]) {
            if (recv_tab_[i]) {
                *status = '.';
            } else {
                *status = 'r';
                n_repaired++;
                n_lost++;
            }
        } else {
            if (i < sblen_) {
                *status = 'X';
            } else {
                *status = 'x';
            }
            n_lost++;
        }
    }

    if (n_lost == 0) {
        return;
    }

    roc_log(LogDebug, "openfec decoder: repaired %u/%u/%u %s", (unsigned)n_repaired,
            (unsigned)n_lost, (unsigned)buff_tab_.size(), &status_[0]);
}

// OpenFEC may allocate memory without calling source_cb_()
// we need our own buffers, so we handle this case here
void OpenfecDecoder::fix_buffer_(size_t index) {
    if (!buff_tab_[index] && data_tab_[index]) {
        roc_log(LogTrace, "openfec decoder: copy buffer: index=%lu",
                (unsigned long)index);

        if (void* buff = make_buffer_(index)) {
            memcpy(buff, data_tab_[index], payload_size_);
        }
    }
}

void* OpenfecDecoder::make_buffer_(size_t index) {
    core::Slice<uint8_t> buffer = packet_factory_.new_packet_buffer();

    if (!buffer) {
        roc_log(LogError, "openfec decoder: can't allocate buffer");
        return NULL;
    }

    if (buffer.capacity() < payload_size_) {
        roc_log(LogError, "openfec decoder: packet size too large: size=%lu max=%lu",
                (unsigned long)payload_size_, (unsigned long)buffer.capacity());
        return NULL;
    }

    buffer.reslice(0, payload_size_);
    buff_tab_[index] = buffer;

    return buffer.data();
}

// called when OpenFEC allocates a source packet
void* OpenfecDecoder::source_cb_(void* context, uint32_t size, uint32_t index) {
    roc_log(LogTrace, "openfec decoder: source callback: index=%lu",
            (unsigned long)index);

    roc_panic_if(context == NULL);
    (void)size;

    OpenfecDecoder& self = *(OpenfecDecoder*)context;
    return self.make_buffer_(index);
}

// called when OpenFEC created a repair packet
// the return value is ignored in OpenFEC
void* OpenfecDecoder::repair_cb_(void* context, uint32_t size, uint32_t index) {
    roc_log(LogTrace, "openfec decoder: repair callback: index=%lu",
            (unsigned long)index);

    roc_panic_if(context == NULL);
    (void)size;
    (void)index;

    return NULL;
}

} // namespace fec
} // namespace roc
