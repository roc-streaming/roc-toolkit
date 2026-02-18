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

namespace roc {
namespace pipeline {

ReceiverSlot::ReceiverSlot(const ReceiverSourceConfig& source_config,
                           const ReceiverSlotConfig& slot_config,
                           StateTracker& state_tracker,
                           audio::Mixer& mixer,
                           audio::ProcessorMap& processor_map,
                           rtp::EncodingMap& encoding_map,
                           packet::PacketFactory& packet_factory,
                           audio::FrameFactory& frame_factory,
                           core::IArena& arena,
                           dbgio::CsvDumper* dumper)
    : core::RefCounted<ReceiverSlot, core::ArenaAllocation>(arena)
    , encoding_map_(encoding_map)
    , state_tracker_(state_tracker)
    , session_group_(source_config,
                     slot_config,
                     state_tracker_,
                     mixer,
                     processor_map,
                     encoding_map,
                     packet_factory,
                     frame_factory,
                     arena,
                     dumper)
    , init_status_(status::NoStatus) {
    roc_log(LogDebug, "receiver slot: initializing");

    if ((init_status_ = session_group_.init_status()) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode ReceiverSlot::init_status() const {
    return init_status_;
}

ReceiverEndpoint* ReceiverSlot::add_endpoint(address::Interface iface,
                                             address::Protocol proto,
                                             const address::SocketAddr& inbound_address,
                                             packet::IWriter* outbound_writer) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_log(LogDebug, "receiver slot: adding %s endpoint %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    switch (iface) {
    case address::Iface_AudioSource:
        return create_source_endpoint_(proto, inbound_address, outbound_writer);

    case address::Iface_AudioRepair:
        return create_repair_endpoint_(proto, inbound_address, outbound_writer);

    case address::Iface_AudioControl:
        return create_control_endpoint_(proto, inbound_address, outbound_writer);

    default:
        break;
    }

    roc_log(LogError, "receiver slot: unsupported interface");
    return NULL;
}

status::StatusCode ReceiverSlot::refresh(core::nanoseconds_t current_time,
                                         core::nanoseconds_t& next_deadline) {
    roc_panic_if(init_status_ != status::StatusOK);

    status::StatusCode code = status::NoStatus;

    if (source_endpoint_) {
        if ((code = source_endpoint_->pull_packets(current_time)) != status::StatusOK) {
            return code;
        }
    }

    if (repair_endpoint_) {
        if ((code = repair_endpoint_->pull_packets(current_time)) != status::StatusOK) {
            return code;
        }
    }

    if (control_endpoint_) {
        if ((code = control_endpoint_->pull_packets(current_time)) != status::StatusOK) {
            return code;
        }
    }

    if ((code = session_group_.refresh_sessions(current_time, next_deadline))
        != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

void ReceiverSlot::reclock(core::nanoseconds_t playback_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    session_group_.reclock_sessions(playback_time);
}

size_t ReceiverSlot::num_sessions() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return session_group_.num_sessions();
}

void ReceiverSlot::get_metrics(ReceiverSlotMetrics& slot_metrics,
                               ReceiverParticipantMetrics* party_metrics,
                               size_t* party_count) const {
    roc_panic_if(init_status_ != status::StatusOK);

    session_group_.get_slot_metrics(slot_metrics);

    if (party_metrics || party_count) {
        session_group_.get_participant_metrics(party_metrics, party_count);
    }
}

ReceiverEndpoint*
ReceiverSlot::create_source_endpoint_(address::Protocol proto,
                                      const address::SocketAddr& inbound_address,
                                      packet::IWriter* outbound_writer) {
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
        proto, state_tracker_, session_group_, encoding_map_, inbound_address,
        outbound_writer, arena()));

    if (!source_endpoint_ || source_endpoint_->init_status() != status::StatusOK) {
        // TODO(gh-183): forward status (control ops)
        roc_log(LogError, "receiver slot: can't create source endpoint");
        source_endpoint_.reset(NULL);
        return NULL;
    }

    return source_endpoint_.get();
}

ReceiverEndpoint*
ReceiverSlot::create_repair_endpoint_(address::Protocol proto,
                                      const address::SocketAddr& inbound_address,
                                      packet::IWriter* outbound_writer) {
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
        proto, state_tracker_, session_group_, encoding_map_, inbound_address,
        outbound_writer, arena()));

    if (!repair_endpoint_ || repair_endpoint_->init_status() != status::StatusOK) {
        // TODO(gh-183): forward status (control ops)
        roc_log(LogError, "receiver slot: can't create repair endpoint");
        repair_endpoint_.reset(NULL);
        return NULL;
    }

    return repair_endpoint_.get();
}

ReceiverEndpoint*
ReceiverSlot::create_control_endpoint_(address::Protocol proto,
                                       const address::SocketAddr& inbound_address,
                                       packet::IWriter* outbound_writer) {
    if (control_endpoint_) {
        roc_log(LogError, "receiver slot: audio control endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioControl, proto)) {
        return NULL;
    }

    control_endpoint_.reset(new (control_endpoint_) ReceiverEndpoint(
        proto, state_tracker_, session_group_, encoding_map_, inbound_address,
        outbound_writer, arena()));

    if (!control_endpoint_ || control_endpoint_->init_status() != status::StatusOK) {
        // TODO(gh-183): forward status (control ops)
        roc_log(LogError, "receiver slot: can't create control endpoint");
        control_endpoint_.reset(NULL);
        return NULL;
    }

    if (session_group_.create_control_pipeline(control_endpoint_.get())
        != status::StatusOK) {
        // TODO(gh-183): forward status (control ops)
        roc_log(LogError, "receiver slot: can't create control pipeline");
        control_endpoint_.reset(NULL);
        return NULL;
    }

    return control_endpoint_.get();
}

} // namespace pipeline
} // namespace roc
