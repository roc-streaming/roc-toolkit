/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

extern "C" {
#include <stdlib.h>
#include <of_mem.h>
}

#include "roc_config/config.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_fec/of_block_decoder.h"

namespace roc {
namespace fec {

namespace {

const size_t SYMB_SZ = ROC_CONFIG_DEFAULT_PACKET_SIZE;

} // namespace

OFBlockDecoder::OFBlockDecoder(const Config& config, core::IByteBufferComposer& composer)
    : blk_source_packets_(config.n_source_packets)
    , blk_repair_packets_(config.n_repair_packets)
    , of_sess_(NULL)
    , of_sess_params_(NULL)
    , composer_(composer)
    , buff_tab_(blk_source_packets_ + blk_repair_packets_)
    , data_tab_(blk_source_packets_ + blk_repair_packets_)
    , recv_tab_(blk_source_packets_ + blk_repair_packets_)
    , has_new_packets_(false)
    , decoding_finished_(false) {
    if (config.codec == ReedSolomon2m) {
        roc_log(LogDebug, "initializing Reed-Solomon decoder");

        codec_id_ = OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
        codec_params_.rs_params_.m = config.rs_m;

        of_sess_params_ = (of_parameters_t*)&codec_params_.rs_params_;
    } else if (config.codec == LDPCStaircase) {
        roc_log(LogDebug, "initializing LDPC decoder");

        codec_id_ = OF_CODEC_LDPC_STAIRCASE_STABLE;
        codec_params_.ldpc_params_.prng_seed = config.ldpc_prng_seed;
        codec_params_.ldpc_params_.N1 = config.ldpc_N1;

        of_sess_params_ = (of_parameters_t*)&codec_params_.ldpc_params_;
    } else {
        roc_panic("block decoder: invalid codec");
    }

    of_sess_params_->nb_source_symbols = (uint32_t)blk_source_packets_;
    of_sess_params_->nb_repair_symbols = (uint32_t)blk_repair_packets_;
    of_sess_params_->encoding_symbol_length = SYMB_SZ;
    of_verbosity = 0;

    OFBlockDecoder::reset(); // non-virtual call from ctor
}

OFBlockDecoder::~OFBlockDecoder() {
    if (of_sess_ != NULL) {
        destroy_session_();
    }
}

size_t OFBlockDecoder::n_data_packets() const {
    return blk_source_packets_;
}

size_t OFBlockDecoder::n_fec_packets() const {
    return blk_repair_packets_;
}

void OFBlockDecoder::reset() {
    if (of_sess_ != NULL) {
        report_();
        destroy_session_();
    }

    reset_session_();

    has_new_packets_ = false;
    decoding_finished_ = false;

    for (size_t i = 0; i < buff_tab_.size(); ++i) {
        buff_tab_[i] = core::IByteBufferConstSlice();
        data_tab_[i] = NULL;
        recv_tab_[i] = false;
    }
}

void OFBlockDecoder::write(size_t index, const core::IByteBufferConstSlice& buffer) {
    if (index >= blk_source_packets_ + blk_repair_packets_) {
        roc_panic("block decoder: index out of bounds: index=%lu, size=%lu",
                  (unsigned long)index,
                  (unsigned long)(blk_source_packets_ + blk_repair_packets_));
    }

    if (!buffer) {
        roc_panic("block decoder: NULL buffer");
    }

    if (buffer.size() != SYMB_SZ) {
        roc_panic("block decoder: invalid payload size: size=%lu, expected=%lu",
                  (unsigned long)buffer.size(), (unsigned long)SYMB_SZ);
    }

    if (buff_tab_[index]) {
        roc_panic("block decoder: can't overwrite buffer: index=%lu",
                  (unsigned long)index);
    }

    has_new_packets_ = true;

    buff_tab_[index] = buffer;
    data_tab_[index] = const_cast<void*>((const void*)buffer.data());
    recv_tab_[index] = true;

    // register new packet and try to repair more packets
    if (of_decode_with_new_symbol(
            of_sess_, data_tab_[index], (unsigned int)index) != OF_STATUS_OK) {
        roc_panic("block decoder: can't add packet to OF session");
    }
}

core::IByteBufferConstSlice OFBlockDecoder::repair(size_t index) {
    if (!buff_tab_[index]) {
        update_();
        fix_buffer_(index);
    }
    return buff_tab_[index];
}

void OFBlockDecoder::update_() {
    roc_panic_if(of_sess_ == NULL);

    if (!has_new_packets_) {
        return;
    }

    decode_();

    of_get_source_symbols_tab(of_sess_, &data_tab_[0]);

    has_new_packets_ = false;
}

void OFBlockDecoder::decode_() {
    if (decoding_finished_ && is_optimal_()) {
        return;
    }

    if (!has_n_packets_(blk_source_packets_)) {
        return;
    }

    if (decoding_finished_) {
        // it's not allowed to decode twice, so we recrate the session
        reset_session_();

        if (of_set_available_symbols(of_sess_, &data_tab_[0]) != OF_STATUS_OK) {
            roc_panic("block decoder: can't add packets to OF session");
        }
    }

    // try to repair more packets
    if (of_finish_decoding(of_sess_) != OF_STATUS_OK) {
        roc_panic("block decoder: can't finish decoding");
    }

    decoding_finished_ = true;
}

// note: we have to calculate this every time because OpenFEC
// doesn't always report to us when it repairs a packet
bool OFBlockDecoder::has_n_packets_(size_t n_packets) const {
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
bool OFBlockDecoder::is_optimal_() const {
    return codec_id_ == OF_CODEC_REED_SOLOMON_GF_2_M_STABLE;
}

void OFBlockDecoder::reset_session_() {
    if (of_sess_ != NULL) {
        of_release_codec_instance(of_sess_);
        of_sess_ = NULL;
    }

    if (OF_STATUS_OK != of_create_codec_instance(&of_sess_, codec_id_, OF_DECODER, 0)) {
        roc_panic("block decoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_sess_ == NULL);

    if (OF_STATUS_OK != of_set_fec_parameters(of_sess_, of_sess_params_)) {
        roc_panic("block decoder: of_set_fec_parameters() failed");
    }

    if (OF_STATUS_OK
        != of_set_callback_functions(
               of_sess_, source_cb_,
               // OpenFEC doesn't repair fec-packets in case of Reed-Solomon FEC
               // and prints curses to the console if we give him the callback for that
               codec_id_ == OF_CODEC_REED_SOLOMON_GF_2_M_STABLE ? NULL : repair_cb_,
               (void*)this)) {
        roc_panic("block decoder: of_set_callback_functions() failed");
    }
}

void OFBlockDecoder::destroy_session_() {
    of_release_codec_instance(of_sess_);
    of_sess_ = NULL;

    // OpenFEC may allocate memory without calling source_cb_()
    // we should free() such memory manually
    for (size_t i = 0; i < blk_source_packets_; i++) {
        if (data_tab_[i] == NULL) {
            continue;
        }
        if (buff_tab_[i] && buff_tab_[i].data() == data_tab_[i]) {
            continue;
        }
        of_free(data_tab_[i]);
        data_tab_[i] = NULL;
    }
}

void OFBlockDecoder::report_() const {
    size_t n_lost = 0, n_repaired = 0;

    char status1[ROC_CONFIG_MAX_FEC_BLOCK_DATA_PACKETS + 1] = {};
    char status2[ROC_CONFIG_MAX_FEC_BLOCK_REDUNDANT_PACKETS + 1] = {};

    roc_panic_if(buff_tab_.size() > (ROC_CONFIG_MAX_FEC_BLOCK_DATA_PACKETS
                                    + ROC_CONFIG_MAX_FEC_BLOCK_REDUNDANT_PACKETS));

    for (size_t i = 0; i < buff_tab_.size(); ++i) {
        char* status =
            (i < blk_source_packets_ ? &status1[i] : &status2[i - blk_source_packets_]);

        if (buff_tab_[i] || data_tab_[i]) {
            if (recv_tab_[i]) {
                *status = '.';
            } else {
                *status = 'r';
                n_repaired++;
                n_lost++;
            }
        } else {
            if (i < blk_source_packets_) {
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

    roc_log(LogDebug, "OFBlockDecoder repaired %u/%u/%u %s %s",
            (unsigned)n_repaired,       //
            (unsigned)n_lost,           //
            (unsigned)buff_tab_.size(), //
            status1,                    //
            status2);
}

// OpenFEC may allocate memory without calling source_cb_()
// we need our own buffers, so we handle this case here
void OFBlockDecoder::fix_buffer_(size_t index) {
    if (!buff_tab_[index] && data_tab_[index]) {
        if (void* buff = make_buffer_(index)) {
            memcpy(buff, data_tab_[index], SYMB_SZ);
        }
    }
}

void* OFBlockDecoder::make_buffer_(size_t index) {
    if (core::IByteBufferPtr buffer = composer_.compose()) {
        buffer->set_size(SYMB_SZ);
        buff_tab_[index] = *buffer;
        return buffer->data();
    } else {
        roc_log(LogDebug, "OF_BlockDecoder: can't allocate buffer");
        return NULL;
    }
}

// called when OpenFEC allocates a source packet
void* OFBlockDecoder::source_cb_(void* context, uint32_t size, uint32_t index) {
    roc_panic_if(context == NULL);
    roc_panic_if(size != SYMB_SZ);

    OFBlockDecoder& self = *(OFBlockDecoder*)context;
    return self.make_buffer_(index);
}

// called when OpenFEC created a repair packet
// the return value is ignored
void* OFBlockDecoder::repair_cb_(void*, uint32_t, uint32_t) {
    return NULL;
}

} // namespace fec
} // namespace roc
