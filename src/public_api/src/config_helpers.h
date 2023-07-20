/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PUBLIC_API_CONFIG_HELPERS_H_
#define ROC_PUBLIC_API_CONFIG_HELPERS_H_

#include "roc/config.h"

#include "roc_peer/context.h"
#include "roc_peer/receiver.h"
#include "roc_peer/sender.h"

namespace roc {
namespace api {

bool context_config_from_user(peer::ContextConfig& out, const roc_context_config& in);

bool sender_config_from_user(peer::Context& context,
                             pipeline::SenderConfig& out,
                             const roc_sender_config& in);

bool receiver_config_from_user(peer::Context& context,
                               pipeline::ReceiverConfig& out,
                               const roc_receiver_config& in);

bool sample_spec_from_user(audio::SampleSpec& out, const roc_media_encoding& in);

bool channel_set_from_user(audio::ChannelSet& out,
                           roc_channel_layout in,
                           unsigned int in_tracks);

bool clock_source_from_user(bool& out_timing, roc_clock_source in);

bool resampler_backend_from_user(audio::ResamplerBackend& out, roc_resampler_backend in);
bool resampler_profile_from_user(audio::ResamplerProfile& out, roc_resampler_profile in);

bool packet_encoding_from_user(unsigned& out_pt, roc_packet_encoding in);
bool fec_encoding_from_user(packet::FecScheme& out, roc_fec_encoding in);

bool interface_from_user(address::Interface& out, const roc_interface& in);

bool proto_from_user(address::Protocol& out, const roc_protocol& in);
bool proto_to_user(roc_protocol& out, address::Protocol in);

} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_CONFIG_HELPERS_H_
