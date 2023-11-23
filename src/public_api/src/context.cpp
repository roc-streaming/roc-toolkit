/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/context.h"

#include "adapters.h"
#include "arena.h"

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_node/context.h"

using namespace roc;

int roc_context_open(const roc_context_config* config, roc_context** result) {
    roc_log(LogInfo, "roc_context_open(): opening context");

    if (!result) {
        roc_log(LogError, "roc_context_open(): invalid arguments: result is null");
        return -1;
    }

    if (!config) {
        roc_log(LogError, "roc_context_open(): invalid arguments: config is null");
        return -1;
    }

    node::ContextConfig imp_config;
    if (!api::context_config_from_user(imp_config, *config)) {
        roc_log(LogError, "roc_context_open(): invalid arguments: bad config");
        return -1;
    }

    core::ScopedPtr<node::Context> imp_context(
        new (api::default_arena) node::Context(imp_config, api::default_arena),
        api::default_arena);

    if (!imp_context) {
        roc_log(LogError, "roc_context_open(): can't allocate context");
        return -1;
    }

    if (!imp_context->is_valid()) {
        roc_log(LogError, "roc_context_open(): can't initialize context");
        return -1;
    }

    *result = (roc_context*)imp_context.release();
    return 0;
}

int roc_context_register_encoding(roc_context* context,
                                  int encoding_id,
                                  const roc_media_encoding* encoding) {
    if (!context) {
        roc_log(LogError,
                "roc_context_register_encoding(): invalid arguments: context is null");
        return -1;
    }

    if (encoding_id < 1 || encoding_id > 127) {
        roc_log(
            LogError,
            "roc_context_register_encoding(): invalid arguments: encoding_id is invalid:"
            " got=%d expected=[1; 127]",
            encoding_id);
        return -1;
    }

    if (!encoding) {
        roc_log(LogError,
                "roc_context_register_encoding(): invalid arguments: encoding is null");
        return -1;
    }

    node::Context* imp_context = (node::Context*)context;

    rtp::Encoding enc;

    enc.payload_type = (unsigned)encoding_id;
    enc.packet_flags = packet::Packet::FlagAudio;

    enc.pcm_format.code = audio::PcmCode_SInt16;
    enc.pcm_format.endian = audio::PcmEndian_Big;

    if (!api::sample_spec_from_user(enc.sample_spec, *encoding)) {
        roc_log(
            LogError,
            "roc_context_register_encoding(): invalid arguments: encoding is invalid");
        return -1;
    }

    enc.new_encoder = &audio::PcmEncoder::construct;
    enc.new_decoder = &audio::PcmDecoder::construct;

    if (!imp_context->encoding_map().add_encoding(enc)) {
        roc_log(LogError, "roc_context_register_encoding(): failed to register encoding");
        return -1;
    }

    return 0;
}

int roc_context_close(roc_context* context) {
    if (!context) {
        roc_log(LogError, "roc_context_close(): invalid arguments: context is null");
        return -1;
    }

    node::Context* imp_context = (node::Context*)context;

    if (imp_context->getref() != 0) {
        roc_log(LogError, "roc_context_close(): context is still in use");
        return -1;
    }

    api::default_arena.destroy_object(*imp_context);

    roc_log(LogInfo, "roc_context_close(): closed context");

    return 0;
}
