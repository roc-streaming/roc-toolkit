/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/control_interface_map.h"
#include "roc_core/log.h"

namespace roc {
namespace ctl {

ControlInterfaceMap::ControlInterfaceMap() {
}

core::SharedPtr<BasicControlEndpoint>
ControlInterfaceMap::new_endpoint(address::Interface iface,
                                  address::Protocol proto,
                                  ControlTaskQueue& task_queue,
                                  netio::NetworkLoop& network_loop,
                                  core::IArena& arena) {
    switch (iface) {
    case address::Iface_AudioControl:
        switch (proto) {
        default:
            break;
        }

        (void)task_queue;
        (void)network_loop;
        (void)arena;

        roc_log(LogError,
                "control endpoint map: unsupported protocol %s for interface %s",
                address::proto_to_str(proto), address::interface_to_str(iface));
        return NULL;

    default:
        break;
    }

    roc_log(LogError, "control endpoint map: unsupported interface %s",
            address::interface_to_str(iface));
    return NULL;
}

} // namespace ctl
} // namespace roc
