/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/sender.h"

#include "adapters.h"

#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_node/sender.h"

using namespace roc;

int roc_sender_open(roc_context* context,
                    const roc_sender_config* config,
                    roc_sender** result) {
    roc_log(LogInfo, "roc_sender_open(): opening sender");

    if (!result) {
        roc_log(LogError, "roc_sender_open(): invalid arguments: result is null");
        return -1;
    }

    if (!context) {
        roc_log(LogError, "roc_sender_open(): invalid arguments: context is null");
        return -1;
    }

    node::Context* imp_context = (node::Context*)context;

    if (!config) {
        roc_log(LogError, "roc_sender_open(): invalid arguments: config is null");
        return -1;
    }

    pipeline::SenderSinkConfig imp_config;
    if (!api::sender_config_from_user(*imp_context, imp_config, *config)) {
        roc_log(LogError, "roc_sender_open(): invalid arguments: bad config");
        return -1;
    }

    core::ScopedPtr<node::Sender> imp_sender(new (imp_context->arena())
                                                 node::Sender(*imp_context, imp_config),
                                             imp_context->arena());

    if (!imp_sender) {
        roc_log(LogError, "roc_sender_open(): can't allocate sender");
        return -1;
    }

    if (!imp_sender->is_valid()) {
        roc_log(LogError, "roc_sender_open(): can't initialize sender");
        return -1;
    }

    *result = (roc_sender*)imp_sender.release();
    return 0;
}

int roc_sender_configure(roc_sender* sender,
                         roc_slot slot,
                         roc_interface iface,
                         const roc_interface_config* config) {
    if (!sender) {
        roc_log(LogError, "roc_sender_configure(): invalid arguments: sender is null");
        return -1;
    }

    node::Sender* imp_sender = (node::Sender*)sender;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError, "roc_sender_configure(): invalid arguments: bad interface");
        return -1;
    }

    if (!config) {
        roc_log(LogError, "roc_sender_configure(): invalid arguments: config is null");
        return -1;
    }

    netio::UdpConfig imp_config;
    if (!api::interface_config_from_user(imp_config, *config)) {
        roc_log(LogError, "roc_sender_configure(): invalid arguments: bad config");
        return -1;
    }

    if (!imp_sender->configure(slot, imp_iface, imp_config)) {
        roc_log(LogError, "roc_sender_configure(): operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_connect(roc_sender* sender,
                       roc_slot slot,
                       roc_interface iface,
                       const roc_endpoint* endpoint) {
    if (!sender) {
        roc_log(LogError, "roc_sender_connect(): invalid arguments: sender is null");
        return -1;
    }

    node::Sender* imp_sender = (node::Sender*)sender;

    if (!endpoint) {
        roc_log(LogError, "roc_sender_connect(): invalid arguments: endpoint is null");
        return -1;
    }

    const address::EndpointUri& imp_endpoint = *(const address::EndpointUri*)endpoint;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError, "roc_sender_connect(): invalid arguments: bad interface");
        return -1;
    }

    if (!imp_sender->connect(slot, imp_iface, imp_endpoint)) {
        roc_log(LogError, "roc_sender_connect(): operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_query(roc_sender* sender,
                     roc_slot slot,
                     roc_sender_metrics* slot_metrics,
                     roc_connection_metrics* conn_metrics,
                     size_t* conn_metrics_count) {
    if (!sender) {
        roc_log(LogError, "roc_sender_query(): invalid arguments: sender is null");
        return -1;
    }

    if (conn_metrics && !conn_metrics_count) {
        roc_log(LogError,
                "roc_sender_query(): invalid arguments:"
                " conn_metrics is non-null, but conn_metrics_count is null");
        return -1;
    }

    node::Sender* imp_sender = (node::Sender*)sender;

    if (!imp_sender->get_metrics(slot, api::sender_slot_metrics_to_user, slot_metrics,
                                 api::sender_participant_metrics_to_user,
                                 conn_metrics_count, conn_metrics)) {
        roc_log(LogError, "roc_sender_query(): operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_unlink(roc_sender* sender, roc_slot slot) {
    if (!sender) {
        roc_log(LogError, "roc_sender_unlink(): invalid arguments: sender is null");
        return -1;
    }

    node::Sender* imp_sender = (node::Sender*)sender;

    if (!imp_sender->unlink(slot)) {
        roc_log(LogError, "roc_sender_unlink(): operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_write(roc_sender* sender, const roc_frame* frame) {
    if (!sender) {
        roc_log(LogError, "roc_sender_write(): invalid arguments: sender is null");
        return -1;
    }

    node::Sender* imp_sender = (node::Sender*)sender;

    sndio::ISink& imp_sink = imp_sender->sink();

    if (!frame) {
        roc_log(LogError, "roc_sender_write(): invalid arguments: frame is null");
        return -1;
    }

    if (frame->samples_size == 0) {
        return 0;
    }

    const size_t factor = imp_sink.sample_spec().num_channels() * sizeof(float);

    if (frame->samples_size % factor != 0) {
        roc_log(LogError,
                "roc_sender_write(): invalid arguments:"
                " # of samples should be multiple of %u",
                (unsigned)factor);
        return -1;
    }

    if (!frame->samples) {
        roc_log(LogError,
                "roc_sender_write(): invalid arguments: frame samples buffer is null");
        return -1;
    }

    audio::Frame imp_frame((float*)frame->samples, frame->samples_size / sizeof(float));
    imp_sink.write(imp_frame);

    return 0;
}

int roc_sender_close(roc_sender* sender) {
    if (!sender) {
        roc_log(LogError, "roc_sender_close(): invalid arguments: sender is null");
        return -1;
    }

    node::Sender* imp_sender = (node::Sender*)sender;
    imp_sender->context().arena().destroy_object(*imp_sender);

    roc_log(LogInfo, "roc_sender_close(): closed sender");

    return 0;
}
