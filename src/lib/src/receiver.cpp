/*
 * Copyright (c) 2017 Roc authors
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
               context.allocator)
    , num_channels(packet::num_channels(cfg.output.channels)) {
}

roc_receiver* roc_receiver_open(roc_context* context, const roc_receiver_config* config) {
    roc_log(LogInfo, "roc_receiver: opening receiver");

    if (!context) {
        roc_log(LogError, "roc_receiver_open: invalid arguments: context == NULL");
        return NULL;
    }

    pipeline::ReceiverConfig rconfig;
    if (config) {
        if (!make_receiver_config(rconfig, *config)) {
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

    ++context->counter;

    return receiver.release();
}

int roc_receiver_bind(roc_receiver* receiver,
                      roc_port_type type,
                      roc_protocol proto,
                      roc_address* address) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: receiver == NULL");
        return -1;
    }

    if (!address) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments: addr == NULL");
        return -1;
    }

    packet::Address& addr = get_address(address);
    if (!addr.valid()) {
        roc_log(LogError, "roc_sender_connect: invalid arguments: invalid address");
        return -1;
    }

    if (!receiver->context.trx.add_udp_receiver(addr, receiver->receiver)) {
        roc_log(LogError, "roc_receiver_bind: bind failed");
        return -1;
    }

    pipeline::PortConfig pconfig;
    if (!make_port_config(pconfig, type, proto, addr)) {
        roc_log(LogError, "roc_receiver_bind: invalid arguments");
        return -1;
    }

    if (!receiver->receiver.add_port(pconfig)) {
        roc_log(LogError, "roc_receiver_bind: can't add pipeline port");
        return -1;
    }

    roc_log(LogInfo, "roc_receiver: bound to %s %s",
            packet::address_to_str(pconfig.address).c_str(),
            pipeline::proto_to_str(pconfig.protocol));

    return 0;
}

int roc_receiver_read(roc_receiver* receiver, roc_frame* frame) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: receiver == NULL");
        return -1;
    }

    if (!frame) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: frame == NULL");
        return -1;
    }

    if (frame->samples_size == 0) {
        return 0;
    }

    const size_t step = receiver->num_channels * sizeof(float);

    if (frame->samples_size % step != 0) {
        roc_log(LogError,
                "roc_receiver_read: invalid arguments: # of samples should be "
                "multiple of # of %u",
                (unsigned)step);
        return -1;
    }

    if (!frame->samples) {
        roc_log(LogError, "roc_receiver_read: invalid arguments: samples == NULL");
        return -1;
    }

    audio::Frame audio_frame((float*)frame->samples, frame->samples_size / sizeof(float));
    receiver->receiver.read(audio_frame);

    return 0;
}

int roc_receiver_close(roc_receiver* receiver) {
    if (!receiver) {
        roc_log(LogError, "roc_receiver_close: invalid arguments: receiver == NULL");
        return -1;
    }

    roc_context& context = receiver->context;

    receiver->receiver.iterate_ports(close_port, receiver);
    receiver->context.allocator.destroy(*receiver);
    --context.counter;

    roc_log(LogInfo, "roc_receiver: closed receiver");

    return 0;
}
