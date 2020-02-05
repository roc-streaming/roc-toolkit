/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_CONFIG_HELPERS_H_
#define ROC_CONFIG_HELPERS_H_

#include "roc/config.h"

#include "roc_peer/context.h"
#include "roc_peer/receiver.h"
#include "roc_peer/sender.h"

namespace roc {
namespace api {

bool context_config_from_user(peer::ContextConfig& out, const roc_context_config& in);

bool sender_config_from_user(pipeline::SenderConfig& out, const roc_sender_config& in);
bool receiver_config_from_user(pipeline::ReceiverConfig& out,
                               const roc_receiver_config& in);

bool interface_from_user(address::Interface& out, roc_port_type in);

bool proto_from_user(address::Protocol& out, roc_protocol in);
bool proto_to_user(roc_protocol& out, address::Protocol in);

} // namespace api
} // namespace roc

#endif // ROC_CONFIG_HELPERS_H_
