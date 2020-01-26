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

    //! Add UDP datagram receiver port.
    //!
    //! Creates a new UDP receiver and bind it to @p bind_address. The receiver
    //! will pass packets to @p writer. Writer will be called from the network
    //! thread. It should not block.
    //!
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected and written
    //! back to @p bind_address.
    //!
    //! @returns
    //!  true on success or false if error occurred
    bool add_udp_receiver(address::SocketAddr& bind_address, packet::IWriter& writer);

    //! Add UDP datagram sender port.
    //!
    //! Creates a new UDP sender, bind to @p bind_address, and returns a writer
    //! that may be used to send packets from this address. Writer may be called
    //! from any thread. It will not block the caller.
    //!
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected and written
    //! back to @p bind_address.
    //!
    //! @returns
    //!  a new packet writer on success or null if error occurred
    packet::IWriter* add_udp_sender(address::SocketAddr& bind_address);

    //! Remove sender or receiver port. Wait until port will be removed.
    void remove_port(address::SocketAddr bind_address);

    //! Resolve endpoint hostname and fill provided address.
    //! @remarks
    //!  Resolved address inherits IP and port from endpoint URI and other attributes
    //!  like multicast and broadcast settings from endpoint object.
    //! @returns
    //!  false if hostname can't be resolved or resolved address is incompatible
    //!  with other endpoint settings (e.g. address is not allowed to be multicast and
    //!  broadcast at the same time).
    bool resolve_endpoint_address(const address::Endpoint& endpoint,
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

        // input and output for port-related tasks
        address::SocketAddr* port_address;
        packet::IWriter* port_writer;
        core::SharedPtr<BasicPort> port;

        // input and output for resolver tasks
        ResolverRequest resolve_req;

        Task()
            : func(NULL)
            , state(TaskPending)
            , port_address(NULL)
            , port_writer(NULL)
            , port(NULL) {
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
