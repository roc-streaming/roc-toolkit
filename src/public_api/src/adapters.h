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
                             pipeline::SenderConfig& out,
                             const roc_sender_config& in);

bool receiver_config_from_user(node::Context& context,
                               pipeline::ReceiverConfig& out,
                               const roc_receiver_config& in);

bool interface_config_from_user(netio::UdpConfig& out, const roc_interface_config& in);

bool sample_spec_from_user(audio::SampleSpec& out,
                           const roc_media_encoding& in,
                           bool is_network);

bool sample_format_from_user(audio::SampleSpec& out, roc_format in, bool is_network);

bool channel_set_from_user(audio::ChannelSet& out,
                           roc_channel_layout in,
                           unsigned int in_tracks);

bool clock_source_from_user(bool& out_timing, roc_clock_source in);

bool clock_sync_backend_from_user(audio::FreqEstimatorInput& out,
                                  roc_clock_sync_backend in);
bool clock_sync_profile_from_user(audio::FreqEstimatorProfile& out,
                                  roc_clock_sync_profile in);

bool resampler_backend_from_user(audio::ResamplerBackend& out, roc_resampler_backend in);
bool resampler_profile_from_user(audio::ResamplerProfile& out, roc_resampler_profile in);

bool packet_encoding_from_user(unsigned& out_pt, roc_packet_encoding in);
bool fec_encoding_from_user(packet::FecScheme& out, roc_fec_encoding in);

bool interface_from_user(address::Interface& out, const roc_interface& in);

bool proto_from_user(address::Protocol& out, const roc_protocol& in);
bool proto_to_user(roc_protocol& out, address::Protocol in);

void receiver_slot_metrics_to_user(roc_receiver_metrics& out,
                                   const pipeline::ReceiverSlotMetrics& in);

void receiver_session_metrics_to_user(
    const pipeline::ReceiverSessionMetrics& sess_metrics,
    size_t sess_index,
    void* sess_arg);

void sender_metrics_to_user(roc_sender_metrics& out,
                            const pipeline::SenderSlotMetrics& in_slot,
                            const pipeline::SenderSessionMetrics& in_sess);

LogLevel log_level_from_user(roc_log_level level);
roc_log_level log_level_to_user(LogLevel level);

void log_message_to_user(roc_log_message& out, const core::LogMessage& in);

} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_ADAPTERS_H_
