/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_slot.h"
#include "roc_core/log.h"
#include "roc_pipeline/endpoint_helpers.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

ReceiverSlot::ReceiverSlot(const ReceiverConfig& receiver_config,
                           ReceiverState& receiver_state,
                           audio::Mixer& mixer,
                           const rtp::EncodingMap& encoding_map,
                           packet::PacketFactory& packet_factory,
                           core::BufferFactory<uint8_t>& byte_buffer_factory,
                           core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                           core::IArena& arena)
    : core::RefCounted<ReceiverSlot, core::ArenaAllocation>(arena)
    , encoding_map_(encoding_map)
    , receiver_state_(receiver_state)
    , session_group_(receiver_config,
                     receiver_state,
                     mixer,
                     encoding_map,
                     packet_factory,
                     byte_buffer_factory,
                     sample_buffer_factory,
                     arena)
    , valid_(false) {
    if (!session_group_.is_valid()) {
        return;
    }

    roc_log(LogDebug, "receiver slot: initializing");

    valid_ = true;
}

bool ReceiverSlot::is_valid() const {
    return valid_;
}

ReceiverEndpoint* ReceiverSlot::add_endpoint(address::Interface iface,
                                             address::Protocol proto) {
    roc_panic_if(!is_valid());

    roc_log(LogDebug, "receiver slot: adding %s endpoint %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    switch (iface) {
    case address::Iface_AudioSource:
        return create_source_endpoint_(proto);

    case address::Iface_AudioRepair:
        return create_repair_endpoint_(proto);

    case address::Iface_AudioControl:
        return create_control_endpoint_(proto);

    default:
        break;
    }

    roc_log(LogError, "receiver slot: unsupported interface");
    return NULL;
}

core::nanoseconds_t ReceiverSlot::refresh(core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    if (source_endpoint_) {
        const status::StatusCode code = source_endpoint_->pull_packets(current_time);
        // TODO(gh-183): forward status
        roc_panic_if(code != status::StatusOK);
    }

    if (repair_endpoint_) {
        const status::StatusCode code = repair_endpoint_->pull_packets(current_time);
        // TODO(gh-183): forward status
        roc_panic_if(code != status::StatusOK);
    }

    if (control_endpoint_) {
        const status::StatusCode code = control_endpoint_->pull_packets(current_time);
        // TODO(gh-183): forward status
        roc_panic_if(code != status::StatusOK);
    }

    return session_group_.refresh_sessions(current_time);
}

void ReceiverSlot::reclock(core::nanoseconds_t playback_time) {
    roc_panic_if(!is_valid());

    session_group_.reclock_sessions(playback_time);
}

size_t ReceiverSlot::num_sessions() const {
    roc_panic_if(!is_valid());

    return session_group_.num_sessions();
}

void ReceiverSlot::get_metrics(ReceiverSlotMetrics& slot_metrics,
                               ReceiverSessionMetrics* sess_metrics,
                               size_t* sess_metrics_size) const {
    roc_panic_if(!is_valid());

    slot_metrics = ReceiverSlotMetrics();
    slot_metrics.num_sessions = session_group_.num_sessions();

    if (sess_metrics) {
        session_group_.get_metrics(sess_metrics, sess_metrics_size);
    }
}

ReceiverEndpoint* ReceiverSlot::create_source_endpoint_(address::Protocol proto) {
    if (source_endpoint_) {
        roc_log(LogError, "receiver slot: audio source endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioSource, proto)) {
        return NULL;
    }

    if (repair_endpoint_) {
        if (!validate_endpoint_pair_consistency(proto, repair_endpoint_->proto())) {
            return NULL;
        }
    }

    source_endpoint_.reset(new (source_endpoint_) ReceiverEndpoint(
        proto, receiver_state_, session_group_, encoding_map_, arena()));

    if (!source_endpoint_ || !source_endpoint_->is_valid()) {
        roc_log(LogError, "receiver slot: can't create source endpoint");
        source_endpoint_.reset(NULL);
        return NULL;
    }

    return source_endpoint_.get();
}

ReceiverEndpoint* ReceiverSlot::create_repair_endpoint_(address::Protocol proto) {
    if (repair_endpoint_) {
        roc_log(LogError, "receiver slot: audio repair endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioRepair, proto)) {
        return NULL;
    }

    if (source_endpoint_) {
        if (!validate_endpoint_pair_consistency(source_endpoint_->proto(), proto)) {
            return NULL;
        }
    }

    repair_endpoint_.reset(new (repair_endpoint_) ReceiverEndpoint(
        proto, receiver_state_, session_group_, encoding_map_, arena()));

    if (!repair_endpoint_ || !repair_endpoint_->is_valid()) {
        roc_log(LogError, "receiver slot: can't create repair endpoint");
        repair_endpoint_.reset(NULL);
        return NULL;
    }

    return repair_endpoint_.get();
}

ReceiverEndpoint* ReceiverSlot::create_control_endpoint_(address::Protocol proto) {
    if (control_endpoint_) {
        roc_log(LogError, "receiver slot: audio control endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioControl, proto)) {
        return NULL;
    }

    control_endpoint_.reset(new (control_endpoint_) ReceiverEndpoint(
        proto, receiver_state_, session_group_, encoding_map_, arena()));

    if (!control_endpoint_ || !control_endpoint_->is_valid()) {
        roc_log(LogError, "receiver slot: can't create control endpoint");
        control_endpoint_.reset(NULL);
        return NULL;
    }

    return control_endpoint_.get();
}

} // namespace pipeline
} // namespace roc
