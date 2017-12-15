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

void close_port(void* arg, const pipeline::PortConfig& port) {
    roc_panic_if_not(arg);
    roc_receiver* receiver = (roc_receiver*)arg;

    receiver->context.trx.remove_port(port.address);
}

} // namespace

roc_receiver::roc_receiver(roc_context& ctx, pipeline::ReceiverConfig& cfg)
    : context(ctx)
    , receiver(cfg,
               format_map,
               context.packet_pool,
               context.byte_buffer_pool,
               context.sample_buffer_pool,
               context.allocator) {
}

roc_receiver* roc_receiver_open(roc_context* context, const roc_receiver_config* config) {
    roc_log(LogInfo, "roc_receiver: opening receiver");

    if (!context) {
        roc_log(LogError, "roc_receiver_open: invalid arguments: context == NULL");
        return NULL;
    }

    pipeline::ReceiverConfig rconfig;
    if (config) {
        if (!config_receiver(rconfig, *config)) {
            roc_log(LogError, "roc_receiver_open: invalid config");
            return NULL;
        }
    }

    core::UniquePtr<roc_receiver> receiver(
        new (context->allocator) roc_receiver(*context, rconfig), context->allocator);

    if (!receiver) {
        roc_log(LogError, "roc_receiver_open: can't allocate receiver pipeline");
        return NULL;
    }

    if (!receiver->receiver.valid()) {
        roc_log(LogError, "roc_receiver_open: can't initialize receiver pipeline");
        return NULL;
    }

    ++context->refcount;

    return receiver.release();
}

int roc_receiver_bind(roc_receiver* receiver, roc_protocol proto, struct sockaddr* addr) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: receiver == NULL");
        return -1;
    }

    if (!addr) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: addr == NULL");
        return -1;
    }

    pipeline::PortConfig pconfig;
    if (!config_port(pconfig, proto, addr)) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments");
        return -1;
    }

    if (!receiver->context.trx.add_udp_receiver(pconfig.address, receiver->receiver)) {
        roc_log(LogError, "roc_receiver_bind: bind failed");
        return -1;
    }

    if (!receiver->receiver.add_port(pconfig)) {
        roc_log(LogError, "roc_receiver_bind: can't add pipeline port");
        return -1;
    }

    memcpy(addr, pconfig.address.saddr(), pconfig.address.slen());

    roc_log(LogInfo, "roc_receiver: bound to %s %s",
            packet::address_to_str(pconfig.address).c_str(),
            pipeline::proto_to_str(pconfig.protocol));

    return 0;
}

ssize_t
roc_receiver_read(roc_receiver* receiver, float* samples, const size_t n_samples) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: receiver == NULL");
        return -1;
    }

    if (n_samples == 0) {
        return 0;
    }

    if ((ssize_t)n_samples < 0) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: too much samples");
        return -1;
    }

    if (!samples) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: samples == NULL");
        return -1;
    }

    // FIXME
    core::Slice<audio::sample_t> buf(
        new (receiver->context.sample_buffer_pool)
            core::Buffer<audio::sample_t>(receiver->context.sample_buffer_pool));
    buf.resize(n_samples);

    audio::Frame frame(buf);
    receiver->receiver.read(frame);

    roc_panic_if(sizeof(float) != sizeof(audio::sample_t));
    memcpy(samples, frame.samples().data(), n_samples * sizeof(audio::sample_t));

    return (ssize_t)n_samples;
}

void roc_receiver_close(roc_receiver* receiver) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_close: invalid arguments: receiver == NULL");
        return;
    }

    receiver->receiver.iterate_ports(close_port, receiver);

    --receiver->context.refcount;

    receiver->context.allocator.destroy(*receiver);

    roc_log(LogInfo, "roc_receiver: closed receiver");
}
