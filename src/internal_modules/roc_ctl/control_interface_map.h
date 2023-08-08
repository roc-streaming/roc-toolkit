/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/control_interface_map.h
//! @brief Control interface map.

#ifndef ROC_CTL_CONTROL_INTERFACE_MAP_H_
#define ROC_CTL_CONTROL_INTERFACE_MAP_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/iarena.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/singleton.h"
#include "roc_ctl/basic_control_endpoint.h"
#include "roc_ctl/control_task_queue.h"
#include "roc_netio/network_loop.h"

namespace roc {
namespace ctl {

//! Control interface map.
class ControlInterfaceMap : public core::NonCopyable<> {
public:
    //! Get instance.
    static ControlInterfaceMap& instance() {
        return core::Singleton<ControlInterfaceMap>::instance();
    }

    //! Create control endpoint for given interface and protocol.
    core::SharedPtr<BasicControlEndpoint> new_endpoint(address::Interface iface,
                                                       address::Protocol proto,
                                                       ControlTaskQueue& task_queue,
                                                       netio::NetworkLoop& network_loop,
                                                       core::IArena& arena);

private:
    friend class core::Singleton<ControlInterfaceMap>;

    ControlInterfaceMap();
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_INTERFACE_MAP_H_
