/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/sender.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_pipeline/port_to_str.h"
#include "roc_pipeline/port_utils.h"

namespace roc {
namespace peer {

Sender::Sender(Context& context, const pipeline::SenderConfig& pipeline_config)
    : BasicPeer(context)
    , pipeline_config_(pipeline_config)
    , udp_writer_(NULL) {
    roc_log(LogDebug, "sender peer: initializing");
}

Sender::~Sender() {
    roc_log(LogDebug, "sender peer: deinitializing");

    if (udp_writer_) {
        context_.event_loop().remove_port(bind_address_);
    }
}

bool Sender::valid() {
    return true;
}

bool Sender::bind(address::SocketAddr& addr) {
    core::Mutex::Lock lock(mutex_);

    if (pipeline_) {
        roc_log(LogError, "sender peer: can't bind after first use");
        return false;
    }

    if (udp_writer_) {
        roc_log(LogError, "sender peer: already bound");
        return false;
    }

    udp_writer_ = context_.event_loop().add_udp_sender(addr);
    if (!udp_writer_) {
        roc_log(LogError, "sender peer: bind failed");
        return false;
    }

    bind_address_ = addr;
    roc_log(LogInfo, "sender peer: bound to %s",
            address::socket_addr_to_str(bind_address_).c_str());

    return true;
}

bool Sender::connect(pipeline::PortType port_type,
                     const pipeline::PortConfig& port_config) {
    core::Mutex::Lock lock(mutex_);

    if (pipeline_) {
        roc_log(LogError, "sender peer: can't connect after first use");
        return false;
    }

    switch ((int)port_type) {
    case pipeline::Port_AudioSource:
        return set_source_port_(port_config);

    case pipeline::Port_AudioRepair:
        return set_repair_port_(port_config);

    default:
        break;
    }

    roc_log(LogError, "sender peer: invalid protocol");
    return false;
}

sndio::ISink* Sender::sink() {
    core::Mutex::Lock lock(mutex_);

    if (!ensure_pipeline_()) {
        return NULL;
    }

    return pipeline_.get();
}

bool Sender::set_source_port_(const pipeline::PortConfig& port_config) {
    if (source_port_.protocol != pipeline::Proto_None) {
        roc_log(LogError, "sender peer: audio source port is already set");
        return false;
    }

    if (!pipeline::validate_port(pipeline_config_.fec_encoder.scheme,
                                 port_config.protocol, pipeline::Port_AudioSource)) {
        return false;
    }

    if (repair_port_.protocol != pipeline::Proto_None) {
        if (!pipeline::validate_ports(pipeline_config_.fec_encoder.scheme,
                                      port_config.protocol, repair_port_.protocol)) {
            return false;
        }
    }

    source_port_ = port_config;

    roc_log(LogInfo, "sender peer: set audio source port to %s",
            pipeline::port_to_str(port_config).c_str());

    return true;
}

bool Sender::set_repair_port_(const pipeline::PortConfig& port_config) {
    if (repair_port_.protocol != pipeline::Proto_None) {
        roc_log(LogError, "sender peer: audio repair port is already set");
        return false;
    }

    if (!pipeline::validate_port(pipeline_config_.fec_encoder.scheme,
                                 port_config.protocol, pipeline::Port_AudioRepair)) {
        return false;
    }

    if (source_port_.protocol != pipeline::Proto_None) {
        if (!pipeline::validate_ports(pipeline_config_.fec_encoder.scheme,
                                      source_port_.protocol, port_config.protocol)) {
            return false;
        }
    }

    repair_port_ = port_config;

    roc_log(LogInfo, "sender peer: set audio repair port to %s",
            pipeline::port_to_str(port_config).c_str());

    return true;
}

bool Sender::ensure_pipeline_() {
    if (pipeline_) {
        return true;
    }

    if (!udp_writer_) {
        roc_log(LogError, "sender peer: bind was not called");
        return false;
    }

    if (source_port_.protocol == pipeline::Proto_None) {
        roc_log(LogError, "sender peer: source port is not connected");
        return false;
    }

    if (repair_port_.protocol == pipeline::Proto_None
        && pipeline_config_.fec_encoder.scheme != packet::FEC_None) {
        roc_log(LogError, "sender peer: repair port is not connected");
        return false;
    }

    pipeline_.reset(new (context_.allocator()) pipeline::SenderSink(
                        pipeline_config_, source_port_, *udp_writer_, repair_port_,
                        *udp_writer_, codec_map_, format_map_, context_.packet_pool(),
                        context_.byte_buffer_pool(), context_.sample_buffer_pool(),
                        context_.allocator()),
                    context_.allocator());

    if (!pipeline_) {
        roc_log(LogError, "sender peer: can't allocate pipeline");
        return false;
    }

    if (!pipeline_->valid()) {
        roc_log(LogError, "sender peer: can't initialize pipeline");
        pipeline_.reset();
        return false;
    }

    return true;
}

} // namespace peer
} // namespace roc
