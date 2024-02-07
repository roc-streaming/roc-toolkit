/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/receiver_decoder.h"

#include "adapters.h"

#include "roc_address/protocol.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_node/receiver_decoder.h"
#include "roc_status/code_to_str.h"

using namespace roc;

int roc_receiver_decoder_open(roc_context* context,
                              const roc_receiver_config* config,
                              roc_receiver_decoder** result) {
    roc_log(LogInfo, "roc_receiver_decoder_open(): opening decoder");

    if (!result) {
        roc_log(LogError,
                "roc_receiver_decoder_open(): invalid arguments: result is null");
        return -1;
    }

    if (!context) {
        roc_log(LogError,
                "roc_receiver_decoder_open(): invalid arguments: context is null");
        return -1;
    }

    node::Context* imp_context = (node::Context*)context;

    if (!config) {
        roc_log(LogError,
                "roc_receiver_decoder_open(): invalid arguments: config is null");
        return -1;
    }

    pipeline::ReceiverConfig imp_config;
    if (!api::receiver_config_from_user(*imp_context, imp_config, *config)) {
        roc_log(LogError, "roc_receiver_decoder_open(): invalid arguments: bad config");
        return -1;
    }

    core::ScopedPtr<node::ReceiverDecoder> imp_decoder(
        new (imp_context->arena()) node::ReceiverDecoder(*imp_context, imp_config),
        imp_context->arena());

    if (!imp_decoder) {
        roc_log(LogError, "roc_receiver_decoder_open(): can't allocate decoder");
        return -1;
    }

    if (!imp_decoder->is_valid()) {
        roc_log(LogError, "roc_receiver_decoder_open(): can't initialize decoder");
        return -1;
    }

    *result = (roc_receiver_decoder*)imp_decoder.release();
    return 0;
}

int roc_receiver_decoder_activate(roc_receiver_decoder* decoder,
                                  roc_interface iface,
                                  roc_protocol proto) {
    if (!decoder) {
        roc_log(LogError,
                "roc_receiver_decoder_activate(): invalid arguments: decoder is null");
        return -1;
    }

    node::ReceiverDecoder* imp_decoder = (node::ReceiverDecoder*)decoder;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_receiver_decoder_activate(): invalid arguments: bad interface");
        return -1;
    }

    address::Protocol imp_proto;
    if (!api::proto_from_user(imp_proto, proto)) {
        roc_log(LogError,
                "roc_receiver_decoder_activate(): invalid arguments: bad protocol");
        return -1;
    }

    if (!imp_decoder->activate(imp_iface, imp_proto)) {
        roc_log(LogError, "roc_receiver_decoder_activate(): operation failed");
        return -1;
    }

    return 0;
}

int roc_receiver_decoder_query(roc_receiver_decoder* decoder,
                               roc_receiver_metrics* decoder_metrics,
                               roc_connection_metrics* conn_metrics,
                               size_t* conn_metrics_count) {
    if (!decoder) {
        roc_log(LogError,
                "roc_receiver_decoder_query(): invalid arguments: decoder is null");
        return -1;
    }

    if (conn_metrics && !conn_metrics_count) {
        roc_log(LogError,
                "roc_receiver_decoder_query(): invalid arguments:"
                " conn_metrics is non-null, but conn_metrics_count is null");
        return -1;
    }

    node::ReceiverDecoder* imp_decoder = (node::ReceiverDecoder*)decoder;

    if (!imp_decoder->get_metrics(api::receiver_slot_metrics_to_user, decoder_metrics,
                                  api::receiver_participant_metrics_to_user,
                                  conn_metrics_count, conn_metrics)) {
        roc_log(LogError, "roc_receiver_decoder_query(): operation failed");
        return -1;
    }

    return 0;
}

int roc_receiver_decoder_push(roc_receiver_decoder* decoder,
                              roc_interface iface,
                              const roc_packet* packet) {
    if (!decoder) {
        roc_log(LogError,
                "roc_receiver_decoder_push(): invalid arguments: decoder is null");
        return -1;
    }

    node::ReceiverDecoder* imp_decoder = (node::ReceiverDecoder*)decoder;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_receiver_decoder_push(): invalid arguments: bad interface");
        return -1;
    }

    if (!packet) {
        roc_log(LogError,
                "roc_receiver_decoder_push(): invalid arguments: packet is null");
        return -1;
    }

    if (!packet->bytes) {
        roc_log(LogError,
                "roc_receiver_decoder_push(): invalid arguments: packet bytes are null");
        return -1;
    }

    if (packet->bytes_size == 0) {
        roc_log(
            LogError,
            "roc_receiver_decoder_push(): invalid arguments: packet bytes count is zero");
        return -1;
    }

    core::SharedPtr<core::Buffer<uint8_t> > imp_buffer =
        imp_decoder->context().byte_buffer_factory().new_buffer();
    if (!imp_buffer) {
        roc_log(LogError,
                "roc_receiver_decoder_push(): invalid arguments: can't allocate buffer");
        return -1;
    }

    if (imp_buffer->size() < packet->bytes_size) {
        roc_log(
            LogError,
            "roc_receiver_decoder_push(): provided packet exceeds maximum packet size:"
            " provided=%lu maximum=%lu",
            (unsigned long)packet->bytes_size, (unsigned long)imp_buffer->size());
        return -1;
    }

    core::Slice<uint8_t> imp_slice(*imp_buffer, 0, packet->bytes_size);
    memcpy(imp_slice.data(), packet->bytes, packet->bytes_size);

    packet::PacketPtr imp_packet = imp_decoder->context().packet_factory().new_packet();
    if (!imp_packet) {
        roc_log(LogError,
                "roc_receiver_decoder_push(): invalid arguments: can't allocate packet");
        return -1;
    }

    imp_packet->add_flags(packet::Packet::FlagUDP);
    imp_packet->set_buffer(imp_slice);

    const status::StatusCode code = imp_decoder->write_packet(imp_iface, imp_packet);
    if (code != status::StatusOK) {
        // TODO(gh-183): forward status code to user
        roc_log(LogError,
                "roc_receiver_decoder_push(): can't write packet to decoder: status=%s",
                status::code_to_str(code));

        return -1;
    }

    return 0;
}

int roc_receiver_decoder_pop(roc_receiver_decoder* decoder, roc_frame* frame) {
    if (!decoder) {
        roc_log(LogError,
                "roc_receiver_decoder_pop(): invalid arguments: decoder is null");
        return -1;
    }

    node::ReceiverDecoder* imp_decoder = (node::ReceiverDecoder*)decoder;

    sndio::ISource& imp_source = imp_decoder->source();

    if (!frame) {
        roc_log(LogError, "roc_receiver_decoder_pop(): invalid arguments: frame is null");
        return -1;
    }

    if (frame->samples_size == 0) {
        return 0;
    }

    const size_t factor = imp_source.sample_spec().num_channels() * sizeof(float);

    if (frame->samples_size % factor != 0) {
        roc_log(LogError,
                "roc_receiver_decoder_pop(): invalid arguments:"
                " # of samples should be multiple of %u",
                (unsigned)factor);
        return -1;
    }

    if (!frame->samples) {
        roc_log(LogError,
                "roc_receiver_decoder_pop(): invalid arguments: samples is null");
        return -1;
    }

    audio::Frame imp_frame((float*)frame->samples, frame->samples_size / sizeof(float));

    if (!imp_source.read(imp_frame)) {
        roc_log(LogError, "roc_receiver_decoder_pop(): got unexpected eof from source");
        return -1;
    }

    return 0;
}

int roc_receiver_decoder_close(roc_receiver_decoder* decoder) {
    if (!decoder) {
        roc_log(LogError,
                "roc_receiver_decoder_close(): invalid arguments: decoder is null");
        return -1;
    }

    node::ReceiverDecoder* imp_decoder = (node::ReceiverDecoder*)decoder;
    imp_decoder->context().arena().destroy_object(*imp_decoder);

    roc_log(LogInfo, "roc_receiver_decoder_close(): closed decoder");

    return 0;
}
