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
#include "roc_fec/ldpc_block_encoder.h"

namespace roc {
namespace fec {

namespace {

const size_t SYMB_SZ = ROC_CONFIG_DEFAULT_PACKET_SIZE;

} // namespace

LDPC_BlockEncoder::LDPC_BlockEncoder(core::IByteBufferComposer& composer)
    : of_inst_(NULL)
    , composer_(composer)
    , sym_tab_(N_DATA_PACKETS + N_FEC_PACKETS)
    , buffers_(N_DATA_PACKETS + N_FEC_PACKETS) {
    roc_log(LogDebug, "initializing ldpc encoder");

    of_ldpc_parameters params;

    of_verbosity = 0;

    if (OF_STATUS_OK != of_create_codec_instance(
                            &of_inst_, OF_CODEC_LDPC_STAIRCASE_STABLE, OF_ENCODER, 0)) {
        roc_panic("ldpc encoder: of_create_codec_instance() failed");
    }

    roc_panic_if(of_inst_ == NULL);

    params.nb_source_symbols = N_DATA_PACKETS;
    params.nb_repair_symbols = N_FEC_PACKETS;
    params.encoding_symbol_length = SYMB_SZ;
    params.prng_seed = 1297501556;
    params.N1 = 7;

    if (OF_STATUS_OK != of_set_fec_parameters(of_inst_, (of_parameters_t*)&params)) {
        roc_panic("ldpc encoder: of_set_fec_parameters() failed");
    }
}

LDPC_BlockEncoder::~LDPC_BlockEncoder() {
    of_release_codec_instance(of_inst_);
}

void LDPC_BlockEncoder::write(size_t index, const core::IByteBufferConstSlice& buffer) {
    if (index >= N_DATA_PACKETS) {
        roc_panic("ldpc encoder: can't write more than %lu data buffers",
                  (unsigned long)N_DATA_PACKETS);
    }

    if (!buffer) {
        roc_panic("ldpc encoder: NULL buffer");
    }

    if ((uintptr_t)buffer.data() % 8 != 0) {
        roc_panic("ldpc encoder: buffer data should be 8-byte aligned");
    }

    // const_cast<> is OK since OpenFEC will not modify this buffer.
    sym_tab_[index] = const_cast<uint8_t*>(buffer.data());
    buffers_[index] = buffer;
}

void LDPC_BlockEncoder::commit() {
    for (size_t i = 0; i < N_FEC_PACKETS; ++i) {
        if (core::IByteBufferPtr buffer = composer_.compose()) {
            buffer->set_size(SYMB_SZ);
            sym_tab_[N_DATA_PACKETS + i] = buffer->data();
            buffers_[N_DATA_PACKETS + i] = *buffer;
        } else {
            roc_log(LogDebug, "ldpc encoder: can't allocate buffer");
            sym_tab_[N_DATA_PACKETS + i] = NULL;
        }
    }

    for (size_t i = N_DATA_PACKETS; i < N_DATA_PACKETS + N_FEC_PACKETS; ++i) {
        if (OF_STATUS_OK != of_build_repair_symbol(of_inst_, &sym_tab_[0], (uint32_t)i)) {
            roc_panic("ldpc encoder: of_build_repair_symbol() failed");
        }
    }
}

core::IByteBufferConstSlice LDPC_BlockEncoder::read(size_t index) {
    if (index >= N_FEC_PACKETS) {
        roc_panic("ldpc encoder: can't read more than %lu fec buffers",
                  (unsigned long)N_FEC_PACKETS);
    }

    return buffers_[N_DATA_PACKETS + index];
}

void LDPC_BlockEncoder::reset() {
    for (size_t i = 0; i < buffers_.size(); ++i) {
        sym_tab_[i] = NULL;
        buffers_[i] = core::IByteBufferConstSlice();
    }
}

} // namespace fec
} // namespace roc
