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

bool make_context_config(peer::ContextConfig& out, const roc_context_config& in);

bool make_sender_config(pipeline::SenderConfig& out, const roc_sender_config& in);

bool make_receiver_config(pipeline::ReceiverConfig& out, const roc_receiver_config& in);

bool make_endpoint_type(address::EndpointType& out, roc_port_type in);

bool make_port_config(pipeline::PortConfig& out,
                      roc_port_type type,
                      roc_protocol proto,
                      const address::SocketAddr& addr);

} // namespace api
} // namespace roc

#endif // ROC_CONFIG_HELPERS_H_
