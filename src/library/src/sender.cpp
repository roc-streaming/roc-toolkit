/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/sender.h"

#include "config_helpers.h"

#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_peer/sender.h"

using namespace roc;

int roc_sender_open(roc_context* context,
                    const roc_sender_config* config,
                    roc_sender** result) {
    roc_log(LogInfo, "roc_sender_open: opening sender");

    if (!result) {
        roc_log(LogError, "roc_sender_open: invalid arguments: result is null");
        return -1;
    }

    if (!context) {
        roc_log(LogError, "roc_sender_open: invalid arguments: context is null");
        return -1;
    }

    peer::Context* imp_context = (peer::Context*)context;

    if (!config) {
        roc_log(LogError, "roc_sender_open: invalid arguments: config is null");
        return -1;
    }

    pipeline::SenderConfig imp_config;
    if (!api::sender_config_from_user(imp_config, *config)) {
        roc_log(LogError, "roc_sender_open: invalid arguments: bad config");
        return -1;
    }

    core::ScopedPtr<peer::Sender> imp_sender(new (imp_context->allocator())
                                                 peer::Sender(*imp_context, imp_config),
                                             imp_context->allocator());

    if (!imp_sender) {
        roc_log(LogError, "roc_sender_open: can't allocate sender");
        return -1;
    }

    if (!imp_sender->valid()) {
        roc_log(LogError, "roc_sender_open: can't initialize sender");
        return -1;
    }

    *result = (roc_sender*)imp_sender.release();
    return 0;
}

int roc_sender_set_outgoing_address(roc_sender* sender,
                                    roc_interface iface,
                                    const char* ip) {
    if (!sender) {
        roc_log(LogError,
                "roc_sender_set_outgoing_address: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_sender_set_outgoing_address: invalid arguments: bad interface");
        return -1;
    }

    if (!ip) {
        roc_log(LogError,
                "roc_sender_set_outgoing_address: invalid arguments: ip is null");
        return -1;
    }

    if (!imp_sender->set_outgoing_address(imp_iface, ip)) {
        roc_log(LogError, "roc_sender_set_outgoing_address: operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_set_broadcast_enabled(roc_sender* sender,
                                     roc_interface iface,
                                     int enabled) {
    if (!sender) {
        roc_log(LogError,
                "roc_sender_set_broadcast_enabled: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_sender_set_broadcast_enabled: invalid arguments: bad interface");
        return -1;
    }

    if (enabled != 0 && enabled != 1) {
        roc_log(LogError,
                "roc_sender_set_broadcast_enabled:"
                " invalid arguments: invalid flag: should be 0 or 1");
        return -1;
    }

    if (!imp_sender->set_broadcast_enabled(imp_iface, (bool)enabled)) {
        roc_log(LogError, "roc_sender_set_broadcast_enabled: operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_set_squashing_enabled(roc_sender* sender,
                                     roc_interface iface,
                                     int enabled) {
    if (!sender) {
        roc_log(LogError,
                "roc_sender_set_squashing_enabled: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_sender_set_squashing_enabled: invalid arguments: bad interface");
        return -1;
    }

    if (enabled != 0 && enabled != 1) {
        roc_log(LogError,
                "roc_sender_set_squashing_enabled:"
                " invalid arguments: invalid flag: should be 0 or 1");
        return -1;
    }

    if (!imp_sender->set_squashing_enabled(imp_iface, (bool)enabled)) {
        roc_log(LogError, "roc_sender_set_squashing_enabled: operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_connect(roc_sender* sender,
                       roc_interface iface,
                       const roc_endpoint* endpoint) {
    if (!sender) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    if (!endpoint) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: endpoint is null");
        return -1;
    }

    const address::EndpointURI& imp_endpoint = *(const address::EndpointURI*)endpoint;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: bad interface");
        return -1;
    }

    if (!imp_sender->connect(imp_iface, imp_endpoint)) {
        roc_log(LogError, "roc_sender_connect: operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_write(roc_sender* sender, const roc_frame* frame) {
    if (!sender) {
        roc_log(LogError, "roc_sender_write: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    sndio::ISink& imp_sink = imp_sender->sink();

    if (!frame) {
        roc_log(LogError, "roc_sender_write: invalid arguments: frame is null");
        return -1;
    }

    if (frame->samples_size == 0) {
        return 0;
    }

    const size_t factor = imp_sink.num_channels() * sizeof(float);

    if (frame->samples_size % factor != 0) {
        roc_log(LogError,
                "roc_sender_write: invalid arguments: # of samples should be "
                "multiple of # of %u",
                (unsigned)factor);
        return -1;
    }

    if (!frame->samples) {
        roc_log(LogError, "roc_sender_write: invalid arguments: samples is null");
        return -1;
    }

    audio::Frame imp_frame((float*)frame->samples, frame->samples_size / sizeof(float));

    imp_sink.write(imp_frame);

    return 0;
}

int roc_sender_close(roc_sender* sender) {
    if (!sender) {
        roc_log(LogError, "roc_sender_close: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    imp_sender->destroy();

    roc_log(LogInfo, "roc_sender_close: closed sender");

    return 0;
}
