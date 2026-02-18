/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PUBLIC_API_ADAPTERS_H_
#define ROC_PUBLIC_API_ADAPTERS_H_

#include "roc/config.h"
#include "roc/log.h"
#include "roc/metrics.h"

#include "roc_audio/freq_estimator.h"
#include "roc_node/context.h"
#include "roc_node/receiver.h"
#include "roc_node/sender.h"

namespace roc {
namespace api {

bool context_config_from_user(node::ContextConfig& out, const roc_context_config& in);

bool sender_config_from_user(node::Context& context,
                             pipeline::SenderSinkConfig& out,
                             const roc_sender_config& in);

bool receiver_config_from_user(node::Context& context,
                               pipeline::ReceiverSourceConfig& out,
                               const roc_receiver_config& in);

bool interface_config_from_user(netio::UdpConfig& out, const roc_interface_config& in);

bool sample_spec_from_user(audio::SampleSpec& out, const roc_media_encoding& in);
bool sample_spec_to_user(roc_media_encoding& out, const audio::SampleSpec& in);

bool sample_format_from_user(audio::SampleSpec& out, const roc_media_encoding& in);
bool sample_format_to_user(roc_media_encoding& out, const audio::SampleSpec& in);

bool channel_set_from_user(audio::ChannelSet& out,
                           roc_channel_layout in_layout,
                           unsigned int in_tracks);
bool channel_set_to_user(roc_channel_layout& out_layout,
                         unsigned int& out_tracks,
                         const audio::ChannelSet& in);

bool clock_source_from_user(bool& out_timing, roc_clock_source in);

bool latency_tuner_backend_from_user(audio::LatencyTunerBackend& out,
                                     roc_latency_tuner_backend in);
bool latency_tuner_profile_from_user(audio::LatencyTunerProfile& out,
                                     roc_latency_tuner_profile in);

bool resampler_backend_from_user(audio::ResamplerBackend& out, roc_resampler_backend in);
bool resampler_profile_from_user(audio::ResamplerProfile& out, roc_resampler_profile in);

bool plc_backend_from_user(int& out, roc_plc_backend in);

bool packet_encoding_from_user(unsigned& out_pt, roc_packet_encoding in);
bool fec_encoding_from_user(packet::FecScheme& out, roc_fec_encoding in);

bool interface_from_user(address::Interface& out, const roc_interface& in);

bool proto_from_user(address::Protocol& out, const roc_protocol& in);
bool proto_to_user(roc_protocol& out, address::Protocol in);

void receiver_slot_metrics_to_user(const pipeline::ReceiverSlotMetrics& slot_metrics,
                                   void* slot_arg);
void receiver_participant_metrics_to_user(
    const pipeline::ReceiverParticipantMetrics& party_metrics,
    size_t party_index,
    void* party_arg);

void sender_slot_metrics_to_user(const pipeline::SenderSlotMetrics& slot_metrics,
                                 void* slot_arg);
void sender_participant_metrics_to_user(
    const pipeline::SenderParticipantMetrics& party_metrics,
    size_t party_index,
    void* party_arg);

void latency_metrics_to_user(roc_connection_metrics& out,
                             const audio::LatencyMetrics& in);
void link_metrics_to_user(roc_connection_metrics& out, const packet::LinkMetrics& in);

LogLevel log_level_from_user(roc_log_level level);
roc_log_level log_level_to_user(LogLevel level);

void log_message_to_user(roc_log_message& out, const core::LogMessage& in);

} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_ADAPTERS_H_
