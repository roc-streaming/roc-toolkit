/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/event_loop.h
//! @brief Network event loop.

#ifndef ROC_NETIO_EVENT_LOOP_H_
#define ROC_NETIO_EVENT_LOOP_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/cond.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/mutex.h"
#include "roc_core/thread.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/resolver.h"
#include "roc_netio/udp_receiver_port.h"
#include "roc_netio/udp_sender_port.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

//! Network event loop serving multiple ports.
class EventLoop : private ICloseHandler,
                  private IResolverRequestHandler,
                  private core::Thread {
public:
    //! Initialize.
    //!
    //! @remarks
    //!  Start background thread if the object was successfully constructed.
    EventLoop(packet::PacketPool& packet_pool,
              core::BufferPool<uint8_t>& buffer_pool,
              core::IAllocator& allocator);

    //! Destroy. Stop all receivers and senders.
    //!
    //! @remarks
    //!  Wait until background thread finishes.
    virtual ~EventLoop();

    //! Check if transceiver was successfully constructed.
    bool valid() const;

    //! Get number of receiver and sender ports.
    size_t num_ports() const;

    //! Opaque receiver port handle.
    typedef struct PortHandle* PortHandle;

    //! Add UDP datagram receiver port.
    //! @remarks
    //!  - Updates @p config with the actual bind address.
    //!  - Passes received packets to @p writer. It is called from network thread.
    //!    It should not block the caller.
    //! @returns
    //!  created port handle or NULL on error.
    PortHandle add_udp_receiver(UdpReceiverConfig& config, packet::IWriter& writer);

    //! Add UDP datagram sender port.
    //! @remarks
    //!  - Updates @p config with the actual bind address.
    //!  - Sets @p writer to a writer that can be used to send packets from this port.
    //!    It may be calledfrom any thread. It will not block the caller.
    //! @returns
    //!  created port handle or NULL on error.
    PortHandle add_udp_sender(UdpSenderConfig& config, packet::IWriter** writer);

    //! Remove port.
    //! Waits until the port is removed.
    void remove_port(PortHandle handle);

    //! Resolve endpoint hostname and fill provided address.
    //! @remarks
    //!  Resolved address inherits IP and port from endpoint URI and other attributes
    //!  like multicast and broadcast settings from endpoint object.
    //! @returns
    //!  false if hostname can't be resolved or resolved address is incompatible
    //!  with other endpoint settings (e.g. address is not allowed to be multicast and
    //!  broadcast at the same time).
    bool resolve_endpoint_address(const address::EndpointURI& endpoint_uri,
                                  address::SocketAddr& resolved_address);

private:
    enum TaskState { TaskPending, TaskSucceeded, TaskFailed };

    // task lifetime is limited to the public method call like add_udp_receiver()
    // task object is allocated on stack is exists until the task is finished
    // the method which allocated the task blocks until the task is finished
    // and then destroys the task object
    struct Task : core::ListNode {
        // method to be executed on event loop thread
        TaskState (EventLoop::*func)(Task&);

        // task state
        TaskState state;

        // for port-related tasks
        core::SharedPtr<BasicPort> port;
        packet::IWriter* port_writer;

        // for port creation
        UdpSenderConfig* sender_config;
        UdpReceiverConfig* receiver_config;

        // for resolver tasks
        ResolverRequest resolve_req;

        Task()
            : func(NULL)
            , state(TaskPending)
            , port_writer(NULL)
            , sender_config(NULL)
            , receiver_config(NULL) {
        }
    };

    static void task_sem_cb_(uv_async_t* handle);
    static void stop_sem_cb_(uv_async_t* handle);

    virtual void handle_closed(BasicPort&);
    virtual void handle_resolved(ResolverRequest& req);

    virtual void run();

    void close_sems_();
    void async_close_ports_();

    void process_tasks_();
    void run_task_(Task&);

    // task methods
    TaskState add_udp_receiver_(Task&);
    TaskState add_udp_sender_(Task&);
    TaskState remove_port_(Task&);
    TaskState resolve_endpoint_address_(Task&);

    void remove_port_(BasicPort& port);
    void async_close_port_(BasicPort& port);
    void wait_port_closed_(const BasicPort& port);

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;
    core::IAllocator& allocator_;

    bool started_;

    uv_loop_t loop_;
    bool loop_initialized_;

    uv_async_t stop_sem_;
    bool stop_sem_initialized_;

    uv_async_t task_sem_;
    bool task_sem_initialized_;

    // list of tasks to be processed
    core::List<Task, core::NoOwnership> tasks_;

    core::Cond task_cond_;  // signaled when a task is succeeded or failed
    core::Cond close_cond_; // signaled when a port is closed

    Resolver resolver_;

    core::List<BasicPort> open_ports_;
    core::List<BasicPort> closing_ports_;

    // protects all fields
    core::Mutex mutex_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_EVENT_LOOP_H_
