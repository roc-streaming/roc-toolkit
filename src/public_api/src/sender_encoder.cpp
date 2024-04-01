/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/sender_encoder.h"

#include "adapters.h"

#include "roc_address/protocol.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_node/sender_encoder.h"
#include "roc_status/code_to_str.h"

using namespace roc;

int roc_sender_encoder_open(roc_context* context,
                            const roc_sender_config* config,
                            roc_sender_encoder** result) {
    roc_log(LogInfo, "roc_sender_encoder_open(): opening encoder");

    if (!result) {
        roc_log(LogError, "roc_sender_encoder_open(): invalid arguments: result is null");
        return -1;
    }

    if (!context) {
        roc_log(LogError,
                "roc_sender_encoder_open(): invalid arguments: context is null");
        return -1;
    }

    node::Context* imp_context = (node::Context*)context;

    if (!config) {
        roc_log(LogError, "roc_sender_encoder_open(): invalid arguments: config is null");
        return -1;
    }

    pipeline::SenderSinkConfig imp_config;
    if (!api::sender_config_from_user(*imp_context, imp_config, *config)) {
        roc_log(LogError, "roc_sender_encoder_open(): invalid arguments: bad config");
        return -1;
    }

    core::ScopedPtr<node::SenderEncoder> imp_encoder(
        new (imp_context->arena()) node::SenderEncoder(*imp_context, imp_config),
        imp_context->arena());

    if (!imp_encoder) {
        roc_log(LogError, "roc_sender_encoder_open(): can't allocate encoder");
        return -1;
    }

    if (!imp_encoder->is_valid()) {
        roc_log(LogError, "roc_sender_encoder_open(): can't initialize encoder");
        return -1;
    }

    *result = (roc_sender_encoder*)imp_encoder.release();
    return 0;
}

int roc_sender_encoder_activate(roc_sender_encoder* encoder,
                                roc_interface iface,
                                roc_protocol proto) {
    if (!encoder) {
        roc_log(LogError,
                "roc_sender_encoder_activate(): invalid arguments: encoder is null");
        return -1;
    }

    node::SenderEncoder* imp_encoder = (node::SenderEncoder*)encoder;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_sender_encoder_activate(): invalid arguments: bad interface");
        return -1;
    }

    address::Protocol imp_proto;
    if (!api::proto_from_user(imp_proto, proto)) {
        roc_log(LogError,
                "roc_sender_encoder_activate(): invalid arguments: bad protocol");
        return -1;
    }

    if (!imp_encoder->activate(imp_iface, imp_proto)) {
        roc_log(LogError, "roc_sender_encoder_activate(): operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_encoder_query(roc_sender_encoder* encoder,
                             roc_sender_metrics* encoder_metrics,
                             roc_connection_metrics* conn_metrics) {
    if (!encoder) {
        roc_log(LogError,
                "roc_sender_encoder_query(): invalid arguments: encoder is null");
        return -1;
    }

    if (!encoder_metrics) {
        roc_log(LogError,
                "roc_sender_encoder_query(): invalid arguments:"
                " encoder_metrics is null");
        return -1;
    }

    if (!conn_metrics) {
        roc_log(LogError,
                "roc_sender_encoder_query(): invalid arguments:"
                " conn_metrics is null");
        return -1;
    }

    node::SenderEncoder* imp_encoder = (node::SenderEncoder*)encoder;

    if (!imp_encoder->get_metrics(api::sender_slot_metrics_to_user, encoder_metrics,
                                  api::sender_participant_metrics_to_user,
                                  conn_metrics)) {
        roc_log(LogError, "roc_sender_encoder_query(): operation failed");
        return -1;
    }

    return 0;
}

int roc_sender_encoder_push_frame(roc_sender_encoder* encoder, const roc_frame* frame) {
    if (!encoder) {
        roc_log(LogError,
                "roc_sender_encoder_push_frame(): invalid arguments:"
                " encoder is null");
        return -1;
    }

    node::SenderEncoder* imp_encoder = (node::SenderEncoder*)encoder;

    sndio::ISink& imp_sink = imp_encoder->sink();

    if (!frame) {
        roc_log(LogError,
                "roc_sender_encoder_push_frame(): invalid arguments:"
                " frame is null");
        return -1;
    }

    if (frame->samples_size == 0) {
        return 0;
    }

    const size_t factor = imp_sink.sample_spec().num_channels() * sizeof(float);

    if (frame->samples_size % factor != 0) {
        roc_log(LogError,
                "roc_sender_encoder_push_frame(): invalid arguments:"
                " # of samples should be multiple of %u",
                (unsigned)factor);
        return -1;
    }

    if (!frame->samples) {
        roc_log(LogError,
                "roc_sender_encoder_push_frame(): invalid arguments:"
                " frame samples buffer is null");
        return -1;
    }

    audio::Frame imp_frame((float*)frame->samples, frame->samples_size / sizeof(float));
    imp_sink.write(imp_frame);

    return 0;
}

int roc_sender_encoder_push_feedback_packet(roc_sender_encoder* encoder,
                                            roc_interface iface,
                                            const roc_packet* packet) {
    if (!encoder) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet(): invalid arguments:"
                " encoder is null");
        return -1;
    }

    node::SenderEncoder* imp_encoder = (node::SenderEncoder*)encoder;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet(): invalid arguments:"
                " bad interface");
        return -1;
    }

    if (!packet) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet(): invalid arguments:"
                " packet is null");
        return -1;
    }

    if (!packet->bytes) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet(): invalid arguments:"
                " packet bytes buffer is null");
        return -1;
    }

    if (packet->bytes_size == 0) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet(): invalid arguments:"
                " packet bytes count is zero");
        return -1;
    }

    core::SharedPtr<core::Buffer<uint8_t> > imp_buffer =
        imp_encoder->context().byte_buffer_factory().new_buffer();
    if (!imp_buffer) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet():"
                " can't allocate buffer of requested size");
        return -1;
    }

    if (imp_buffer->size() < packet->bytes_size) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet():"
                " provided packet exceeds maximum packet size (see roc_context_config):"
                " provided=%lu maximum=%lu",
                (unsigned long)packet->bytes_size, (unsigned long)imp_buffer->size());
        return -1;
    }

    core::Slice<uint8_t> imp_slice(*imp_buffer, 0, packet->bytes_size);
    memcpy(imp_slice.data(), packet->bytes, packet->bytes_size);

    packet::PacketPtr imp_packet = imp_encoder->context().packet_factory().new_packet();
    if (!imp_packet) {
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet():"
                " can't allocate packet");
        return -1;
    }

    imp_packet->add_flags(packet::Packet::FlagUDP);
    imp_packet->set_buffer(imp_slice);

    const status::StatusCode code = imp_encoder->write_packet(imp_iface, imp_packet);
    if (code != status::StatusOK) {
        // TODO(gh-183): forward status code to user
        roc_log(LogError,
                "roc_sender_encoder_push_feedback_packet():"
                " can't write packet to encoder: status=%s",
                status::code_to_str(code));

        return -1;
    }

    return 0;
}

int roc_sender_encoder_pop_packet(roc_sender_encoder* encoder,
                                  roc_interface iface,
                                  roc_packet* packet) {
    if (!encoder) {
        roc_log(LogError,
                "roc_sender_encoder_pop_packet(): invalid arguments:"
                " encoder is null");
        return -1;
    }

    node::SenderEncoder* imp_encoder = (node::SenderEncoder*)encoder;

    address::Interface imp_iface;
    if (!api::interface_from_user(imp_iface, iface)) {
        roc_log(LogError,
                "roc_sender_encoder_pop_packet(): invalid arguments:"
                " bad interface");
        return -1;
    }

    if (!packet) {
        roc_log(LogError,
                "roc_sender_encoder_pop_packet(): invalid arguments:"
                " packet is null");
        return -1;
    }

    if (!packet->bytes) {
        roc_log(LogError,
                "roc_sender_encoder_pop_packet(): invalid arguments:"
                " packet bytes buffer is null");
        return -1;
    }

    packet::PacketPtr imp_packet;
    const status::StatusCode code = imp_encoder->read_packet(imp_iface, imp_packet);
    if (code != status::StatusOK) {
        // TODO(gh-183): forward status code to user
        if (code != status::StatusNoData) {
            roc_log(LogError,
                    "roc_sender_encoder_pop_packet():"
                    " can't read packet from encoder: status=%s",
                    status::code_to_str(code));
        }
        return -1;
    }

    if (packet->bytes_size < imp_packet->buffer().size()) {
        roc_log(LogError,
                "roc_sender_encoder_pop_packet(): not enough space in provided packet:"
                " provided=%lu needed=%lu",
                (unsigned long)packet->bytes_size,
                (unsigned long)imp_packet->buffer().size());
        return -1;
    }

    memcpy(packet->bytes, imp_packet->buffer().data(), imp_packet->buffer().size());
    packet->bytes_size = imp_packet->buffer().size();

    return 0;
}

int roc_sender_encoder_close(roc_sender_encoder* encoder) {
    if (!encoder) {
        roc_log(LogError,
                "roc_sender_encoder_close(): invalid arguments: encoder is null");
        return -1;
    }

    node::SenderEncoder* imp_encoder = (node::SenderEncoder*)encoder;
    imp_encoder->context().arena().destroy_object(*imp_encoder);

    roc_log(LogInfo, "roc_sender_encoder_close(): closed encoder");

    return 0;
}
