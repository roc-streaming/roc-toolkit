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
    //! Opaque receiver port handle.
    typedef struct PortHandle* PortHandle;

    //! Base task class.
    //! The user is responsible for allocating and deallocating the task.
    class Task : public core::ListNode {
    public:
        //! Check that the task finished and succeeded.
        bool success() const;

    protected:
        friend class EventLoop;

        Task();

        //! Task state.
        enum State { Pending, Closing, Finished };

        void (EventLoop::*func_)(Task&); //!< Task implementation method.

        State state_;  //!< State.
        bool success_; //!< Completion status.

        core::SharedPtr<BasicPort> port_; //!< On which port the task operates.

        PortHandle port_handle_;       //!< Port handle.
        packet::IWriter* port_writer_; //!< Port writer.

        UdpSenderConfig* sender_config_;     //!< Sender port config.
        UdpReceiverConfig* receiver_config_; //!< Receiver port config.

        ResolverRequest resolve_req_; //!< For resolve tasks.
    };

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Add UDP datagram receiver port.
        class AddUdpReceiverPort : public Task {
        public:
            //! Set task parameters.
            //! @remarks
            //!  - Updates @p config with the actual bind address.
            //!  - Passes received packets to @p writer. It is called from network thread.
            //!    It should not block the caller.
            AddUdpReceiverPort(UdpReceiverConfig& config, packet::IWriter& writer);

            //! Get created port handle.
            //! @pre
            //!  Should be called only if success() is true.
            PortHandle get_handle() const;
        };

        //! Add UDP datagram sender port.
        class AddUdpSenderPort : public Task {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Updates @p config with the actual bind address.
            AddUdpSenderPort(UdpSenderConfig& config);

            //! Get created port handle.
            //! @pre
            //!  Should be called only if success() is true.
            PortHandle get_handle() const;

            //! Get created port writer;
            //! @remarks
            //!  The writer can be used to send packets from the port. It may be called
            //!  from any thread. It will not block the caller.
            //! @pre
            //!  Should be called only if success() is true.
            packet::IWriter* get_writer() const;
        };

        //! Remove port.
        class RemovePort : public Task {
        public:
            //! Set task parameters.
            RemovePort(PortHandle handle);
        };

        //! Resolve endpoint address.
        class ResolveEndpointAddress : public Task {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Gets endpoint hostname, resolves it, and writes the resolved IP address
            //!  and the port from the endpoint to the resulting SocketAddr.
            ResolveEndpointAddress(const address::EndpointURI& endpoint_uri);

            //! Get resolved address.
            //! @pre
            //!  Should be called only if success() is true.
            const address::SocketAddr& get_address() const;
        };
    };

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

    //! Enqueue a task for execution and wait for completion.
    //! @returns
    //!  true if the task succeeded or false if it failed.
    bool enqueue_and_wait(Task& task);

private:
    static void task_sem_cb_(uv_async_t* handle);
    static void stop_sem_cb_(uv_async_t* handle);

    virtual void handle_closed(BasicPort&);
    virtual void handle_resolved(ResolverRequest& req);

    virtual void run();

    void close_all_sems_();
    void close_all_ports_();

    void process_pending_tasks_();

    void task_add_udp_receiver_(Task&);
    void task_add_udp_sender_(Task&);
    void task_remove_port_(Task&);
    void task_resolve_endpoint_address_(Task&);

    bool async_close_port_(BasicPort& port);
    void finish_closing_tasks_(const BasicPort& port);

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

    core::List<Task, core::NoOwnership> pending_tasks_;
    core::List<Task, core::NoOwnership> closing_tasks_;

    core::Cond task_cond_;

    Resolver resolver_;

    core::List<BasicPort> open_ports_;
    core::List<BasicPort> closing_ports_;

    core::Mutex mutex_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_EVENT_LOOP_H_
