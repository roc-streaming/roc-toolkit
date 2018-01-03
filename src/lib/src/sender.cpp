/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

#include "roc_core/log.h"
#include "roc_packet/address_to_str.h"
#include "roc_pipeline/proto_to_str.h"

using namespace roc;

namespace {

bool init_pipeline(roc_sender* sender) {
    sender->sender.reset(new (sender->context.allocator) pipeline::Sender(
                             sender->config, *sender->writer, *sender->writer,
                             sender->format_map, sender->context.packet_pool,
                             sender->context.byte_buffer_pool, sender->context.allocator),
                         sender->context.allocator);

    if (!sender->sender) {
        roc_log(LogError, "roc_sender: can't allocate sender pipeline");
        return false;
    }

    if (!sender->sender->valid()) {
        roc_log(LogError, "roc_sender: can't initialize sender pipeline");
        return false;
    }

    return true;
}

bool init_port(roc_sender* sender, const pipeline::PortConfig& pconfig) {
    switch (pconfig.protocol) {
    case pipeline::Proto_None:
        break;

    case pipeline::Proto_RTP:
    case pipeline::Proto_RTP_RSm8_Source:
    case pipeline::Proto_RTP_LDPC_Source:
        if (sender->config.source_port.protocol != pipeline::Proto_None) {
            roc_log(LogError, "roc_sender: source port is already connected");
            return false;
        }

        sender->config.source_port = pconfig;

        roc_log(LogInfo, "roc_sender: connected source port to %s %s",
                packet::address_to_str(pconfig.address).c_str(),
                pipeline::proto_to_str(pconfig.protocol));

        return true;

    case pipeline::Proto_RSm8_Repair:
    case pipeline::Proto_LDPC_Repair:
        if (sender->config.repair_port.protocol != pipeline::Proto_None) {
            roc_log(LogError, "roc_sender: repair port is already connected");
            return false;
        }

        if (sender->config.fec.codec == fec::NoCodec) {
            roc_log(LogError,
                    "roc_sender: repair port can't be used when fec is disabled");
            return false;
        }

        sender->config.repair_port = pconfig;

        roc_log(LogInfo, "roc_sender: connected repair port to %s %s",
                packet::address_to_str(pconfig.address).c_str(),
                pipeline::proto_to_str(pconfig.protocol));

        return true;
    }

    roc_log(LogError, "roc_sender: invalid protocol");
    return false;
}

bool check_connected(roc_sender* sender) {
    if (sender->config.source_port.protocol == pipeline::Proto_None) {
        roc_log(LogError, "roc_sender: source port is not connected");
        return false;
    }

    if (sender->config.repair_port.protocol == pipeline::Proto_None
        && sender->config.fec.codec != fec::NoCodec) {
        roc_log(LogError, "roc_sender: repair port is not connected");
        return false;
    }

    return true;
}

} // namespace

roc_sender::roc_sender(roc_context& ctx, pipeline::SenderConfig& cfg)
    : context(ctx)
    , config(cfg)
    , writer(NULL) {
}

roc_sender* roc_sender_open(roc_context* context, const roc_sender_config* config) {
    roc_log(LogInfo, "roc_sender: opening sender");

    if (!context) {
        roc_log(LogError, "roc_sender_open: invalid arguments: context == NULL");
        return NULL;
    }

    pipeline::SenderConfig sconfig;
    if (config) {
        if (!config_sender(sconfig, *config)) {
            roc_log(LogError, "roc_sender_open: invalid config");
            return NULL;
        }
    }

    roc_sender* sender = new (context->allocator) roc_sender(*context, sconfig);
    if (!sender) {
        roc_log(LogError, "roc_sender_open: can't allocate roc_sender");
        return NULL;
    }

    ++context->counter;

    return sender;
}

int roc_sender_bind(roc_sender* sender, struct sockaddr* src_addr) {
    if (!sender) {
        roc_log(LogError, "roc_sender_bind: invalid arguments: sender == NULL");
        return -1;
    }

    if (!src_addr) {
        roc_log(LogError, "roc_sender_bind: invalid arguments: src_addr == NULL");
        return -1;
    }

    core::Mutex::Lock lock(sender->mutex);

    if (sender->sender) {
        roc_log(LogError, "roc_sender_bind: can't be called after first write");
        return -1;
    }

    if (sender->writer) {
        roc_log(LogError, "roc_sender_bind: sender is already bound");
        return -1;
    }

    packet::Address addr;
    if (!addr.set_saddr(src_addr)) {
        roc_log(LogError, "roc_sender_bind: invalid arguments: bad src_addr");
        return -1;
    }

    sender->writer = sender->context.trx.add_udp_sender(addr);
    if (!sender->writer) {
        roc_log(LogError, "roc_sender_bind: bind failed");
        return -1;
    }

    sender->address = addr;
    memcpy(src_addr, addr.saddr(), addr.slen());

    roc_log(LogInfo, "roc_sender: bound to %s",
            packet::address_to_str(sender->address).c_str());

    return 0;
}

int roc_sender_connect(roc_sender* sender,
                       roc_protocol proto,
                       const struct sockaddr* dst_addr) {
    if (!sender) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: sender == NULL");
        return -1;
    }

    if (!dst_addr) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: dst_addr == NULL");
        return -1;
    }

    core::Mutex::Lock lock(sender->mutex);

    if (sender->sender) {
        roc_log(LogError, "roc_sender_connect: can't be called after first write");
        return -1;
    }

    pipeline::PortConfig pconfig;
    if (!config_port(pconfig, proto, dst_addr)) {
        roc_log(LogError, "roc_sender_connect: invalid arguments");
        return -1;
    }

    if (!init_port(sender, pconfig)) {
        roc_log(LogError, "roc_sender_connect: connect failed");
        return -1;
    }

    return 0;
}

ssize_t
roc_sender_write(roc_sender* sender, const float* samples, const size_t n_samples) {
    if (!sender) {
        roc_log(LogError, "roc_sender_write: invalid arguments: sender == NULL");
        return -1;
    }

    core::Mutex::Lock lock(sender->mutex);

    if (!sender->writer) {
        roc_log(LogError, "roc_sender_write: sender is not properly bound");
        return -1;
    }

    if (!check_connected(sender)) {
        roc_log(LogError, "roc_sender_write: sender is not properly connected");
        return -1;
    }

    if (!sender->sender) {
        if (!init_pipeline(sender)) {
            roc_log(LogError, "roc_sender_write: lazy initialization failed");
            return -1;
        }
    }

    if (!sender->sender->valid()) {
        roc_log(LogError, "roc_sender_write: sender is not properly initialized");
        return -1;
    }

    if (n_samples == 0) {
        return 0;
    }

    if ((ssize_t)n_samples < 0) {
        roc_log(LogError, "roc_sender_write: invalid arguments: too much samples");
        return -1;
    }

    if (!samples) {
        roc_log(LogError, "roc_sender_write: invalid arguments: samples == NULL");
        return -1;
    }

    audio::Frame frame(const_cast<float*>(samples), n_samples);
    sender->sender->write(frame);

    return (ssize_t)n_samples;
}

int roc_sender_close(roc_sender* sender) {
    if (!sender) {
        roc_log(LogError, "roc_sender_close: invalid arguments: sender == NULL");
        return -1;
    }

    if (sender->writer) {
        sender->context.trx.remove_port(sender->address);
    }

    roc_context& context = sender->context;

    sender->context.allocator.destroy(*sender);
    --context.counter;

    roc_log(LogInfo, "roc_sender: closed sender");

    return 0;
}
