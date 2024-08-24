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
#include "plugin_plc.h"

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
        new (api::default_arena) node::Context(imp_config, api::default_arena));

    if (!imp_context) {
        roc_log(LogError, "roc_context_open(): can't allocate context");
        return -1;
    }

    if (imp_context->init_status() != status::StatusOK) {
        roc_log(LogError, "roc_context_open(): can't initialize receiver: status=%s",
                status::code_to_str(imp_context->init_status()));
        return -1;
    }

    *result = (roc_context*)imp_context.hijack();
    return 0;
}

int roc_context_register_encoding(roc_context* context,
                                  int encoding_id,
                                  const roc_media_encoding* encoding) {
    if (!context) {
        roc_log(LogError,
                "roc_context_register_encoding(): invalid arguments:"
                " context is null");
        return -1;
    }

    if (encoding_id < ROC_ENCODING_ID_MIN || encoding_id > ROC_ENCODING_ID_MAX) {
        roc_log(LogError,
                "roc_context_register_encoding(): invalid arguments:"
                " encoding_id out of range: value=%d range=[%d; %d]",
                encoding_id, ROC_ENCODING_ID_MIN, ROC_ENCODING_ID_MAX);
        return -1;
    }

    if (!encoding) {
        roc_log(LogError,
                "roc_context_register_encoding(): invalid arguments:"
                " encoding is null");
        return -1;
    }

    node::Context* imp_context = (node::Context*)context;

    rtp::Encoding enc;

    enc.payload_type = (unsigned int)encoding_id;
    enc.packet_flags = packet::Packet::FlagAudio;

    if (!api::sample_spec_from_user(enc.sample_spec, *encoding)) {
        roc_log(
            LogError,
            "roc_context_register_encoding(): invalid arguments: encoding is invalid");
        return -1;
    }

    const status::StatusCode code = imp_context->encoding_map().register_encoding(enc);

    if (code != status::StatusOK) {
        roc_log(LogError,
                "roc_context_register_encoding(): failed to register encoding: status=%s",
                status::code_to_str(code));
        return -1;
    }

    return 0;
}

int roc_context_register_plc(roc_context* context,
                             int plugin_id,
                             roc_plugin_plc* plugin) {
    if (!context) {
        roc_log(LogError,
                "roc_context_register_plc(): invalid arguments:"
                " context is null");
        return -1;
    }

    if (plugin_id < ROC_PLUGIN_ID_MIN || plugin_id > ROC_PLUGIN_ID_MAX) {
        roc_log(LogError,
                "roc_context_register_plc(): invalid arguments:"
                " plugin_id out of range: value=%d range=[%d; %d]",
                plugin_id, ROC_PLUGIN_ID_MIN, ROC_PLUGIN_ID_MAX);
        return -1;
    }

    if (!api::PluginPlc::validate(plugin)) {
        roc_log(LogError,
                "roc_context_register_plc(): invalid arguments:"
                " invalid function table");
        return -1;
    }

    node::Context* imp_context = (node::Context*)context;

    const status::StatusCode code = imp_context->processor_map().register_plc(
        plugin_id, plugin, &api::PluginPlc::construct);

    if (code != status::StatusOK) {
        roc_log(LogError,
                "roc_context_register_plc(): failed to register encoding: status=%s",
                status::code_to_str(code));
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
        roc_log(LogError,
                "roc_context_close(): can't close context:"
                " there is %d unclosed peer(s) attached to context",
                (int)imp_context->getref());
        return -1;
    }

    api::default_arena.dispose_object(*imp_context);

    roc_log(LogInfo, "roc_context_close(): closed context");

    return 0;
}
