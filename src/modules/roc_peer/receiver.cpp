/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/receiver.h"
#include "roc_address/endpoint_uri_to_str.h"
#include "roc_address/socket_addr.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace peer {

Receiver::Receiver(Context& context, const pipeline::ReceiverConfig& pipeline_config)
    : BasicPeer(context)
    , pipeline_(*this,
                pipeline_config,
                format_map_,
                context_.packet_pool(),
                context_.byte_buffer_pool(),
                context_.sample_buffer_pool(),
                context_.allocator())
    , endpoint_set_(0)
    , process_pipeline_tasks_(pipeline_) {
    roc_log(LogDebug, "receiver peer: initializing");

    if (!pipeline_.valid()) {
        return;
    }

    pipeline::ReceiverSource::Tasks::AddEndpointSet task;
    if (!pipeline_.schedule_and_wait(task)) {
        return;
    }

    endpoint_set_ = task.get_handle();
}

Receiver::~Receiver() {
    roc_log(LogDebug, "receiver peer: deinitializing");

    context_.control_loop().wait(process_pipeline_tasks_);

    for (size_t i = 0; i < ROC_ARRAY_SIZE(ports_); i++) {
        if (ports_[i].handle) {
            netio::NetworkLoop::Tasks::RemovePort task(ports_[i].handle);

            if (!context_.network_loop().schedule_and_wait(task)) {
                roc_panic("receiver peer: can't remove port");
            }
        }
    }
}

bool Receiver::valid() {
    return endpoint_set_;
}

bool Receiver::set_multicast_group(address::Interface iface, const char* ip) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    roc_panic_if(!ip);
    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)ROC_ARRAY_SIZE(ports_));

    if (ports_[iface].handle) {
        roc_log(LogError,
                "receiver peer:"
                " can't set multicast group for %s interface:"
                " interface is already bound",
                address::interface_to_str(iface));
        return false;
    }

    {
        // validation
        address::SocketAddr addr;
        if (!addr.set_host_port_auto(ip, 0)) {
            roc_log(LogError,
                    "receiver peer:"
                    " can't set multicast group for %s interface to '%s':"
                    " invalid IPv4 or IPv6 address",
                    address::interface_to_str(iface), ip);
            return false;
        }
    }

    core::StringBuilder b(ports_[iface].config.multicast_interface,
                          sizeof(ports_[iface].config.multicast_interface));

    if (!b.set_str(ip)) {
        roc_log(LogError,
                "receiver peer:"
                " can't set multicast group for %s interface to '%s':"
                " invalid IPv4 or IPv6 address",
                address::interface_to_str(iface), ip);
        return false;
    }

    roc_log(LogDebug, "receiver peer: setting %s interface multicast group to %s",
            address::interface_to_str(iface), ip);

    return true;
}

bool Receiver::bind(address::Interface iface, address::EndpointURI& uri) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    if (!uri.check(address::EndpointURI::Subset_Full)) {
        roc_log(LogError, "receiver peer: invalid uri");
        return false;
    }

    address::SocketAddr address;

    netio::NetworkLoop::Tasks::ResolveEndpointAddress resolve_task(uri);

    if (!context_.network_loop().schedule_and_wait(resolve_task)) {
        roc_log(LogError, "receiver peer: can't resolve %s interface address",
                address::interface_to_str(iface));
        return false;
    }

    pipeline::ReceiverSource::Tasks::CreateEndpoint endpoint_task(endpoint_set_, iface,
                                                                  uri.proto());
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError, "receiver peer: can't add %s endpoint to pipeline",
                address::interface_to_str(iface));
        return false;
    }

    ports_[iface].config.bind_address = resolve_task.get_address();

    netio::NetworkLoop::Tasks::AddUdpReceiverPort port_task(ports_[iface].config,
                                                            *endpoint_task.get_writer());
    if (!context_.network_loop().schedule_and_wait(port_task)) {
        roc_log(LogError, "receiver peer: can't bind %s interface to local port",
                address::interface_to_str(iface));

        pipeline::ReceiverSource::Tasks::DeleteEndpoint delete_endpoint_task(
            endpoint_set_, iface);
        if (!pipeline_.schedule_and_wait(delete_endpoint_task)) {
            roc_panic("receiver peer: can't remove newly created endpoint");
        }

        return false;
    }

    ports_[iface].handle = port_task.get_handle();

    if (uri.port() == 0) {
        uri.set_port(ports_[iface].config.bind_address.port());
    }

    roc_log(LogInfo, "receiver peer: bound %s interface to %s",
            address::interface_to_str(iface), address::endpoint_uri_to_str(uri).c_str());

    return true;
}

sndio::ISource& Receiver::source() {
    return pipeline_;
}

void Receiver::schedule_task_processing(pipeline::TaskPipeline&,
                                        core::nanoseconds_t deadline) {
    context_.control_loop().reschedule_at(process_pipeline_tasks_, deadline);
}

void Receiver::cancel_task_processing(pipeline::TaskPipeline&) {
    context_.control_loop().async_cancel(process_pipeline_tasks_);
}

} // namespace peer
} // namespace roc
