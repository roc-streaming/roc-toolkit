/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/sender.h"

#include "address_helpers.h"
#include "config_helpers.h"

#include "roc_core/log.h"
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
    if (!api::make_sender_config(imp_config, *config)) {
        roc_log(LogError, "roc_sender_open: invalid arguments: bad config");
        return -1;
    }

    peer::Sender* imp_sender =
        new (imp_context->allocator()) peer::Sender(*imp_context, imp_config);

    if (!imp_sender) {
        roc_log(LogError, "roc_sender_open: can't allocate sender");
        return -1;
    }

    if (!imp_sender->valid()) {
        roc_log(LogError, "roc_sender_open: can't initialize sender");

        delete imp_sender;
        return -1;
    }

    *result = (roc_sender*)imp_sender;
    return 0;
}

int roc_sender_bind(roc_sender* sender, roc_address* address) {
    if (!sender) {
        roc_log(LogError, "roc_sender_bind: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    if (!address) {
        roc_log(LogError, "roc_sender_bind: invalid arguments: address is null");
        return -1;
    }

    address::SocketAddr& imp_address = api::get_socket_addr(address);
    if (!imp_address.has_host_port()) {
        roc_log(LogError, "roc_sender_bind: invalid arguments: invalid address");
        return -1;
    }

    if (!imp_sender->bind(imp_address)) {
        roc_log(LogError, "roc_sender_bind: bind failed");
        return -1;
    }

    return 0;
}

int roc_sender_connect(roc_sender* sender,
                       roc_port_type type,
                       roc_protocol proto,
                       const roc_address* address) {
    if (!sender) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: sender is null");
        return -1;
    }

    peer::Sender* imp_sender = (peer::Sender*)sender;

    if (!address) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: address is null");
        return -1;
    }

    const address::SocketAddr& imp_address = api::get_socket_addr(address);
    if (!imp_address.has_host_port()) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: invalid address");
        return -1;
    }

    address::EndpointType imp_endpoint_type;
    if (!api::make_endpoint_type(imp_endpoint_type, type)) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: bad type");
        return -1;
    }

    address::EndpointProtocol imp_endpoint_proto;
    if (!api::make_endpoint_proto(imp_endpoint_proto, proto)) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: bad protocol");
        return -1;
    }

    if (!imp_sender->connect(imp_endpoint_type, imp_endpoint_proto, imp_address)) {
        roc_log(LogError, "roc_sender_bind: connect failed");
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
