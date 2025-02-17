/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/network_loop.h
//! @brief Network event loop thread.

#ifndef ROC_NETIO_NETWORK_LOOP_H_
#define ROC_NETIO_NETWORK_LOOP_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/atomic.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/list.h"
#include "roc_core/mpsc_queue.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/optional.h"
#include "roc_core/semaphore.h"
#include "roc_core/thread.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn.h"
#include "roc_netio/iconn_acceptor.h"
#include "roc_netio/iconn_handler.h"
#include "roc_netio/inetwork_task_completer.h"
#include "roc_netio/iterminate_handler.h"
#include "roc_netio/network_task.h"
#include "roc_netio/resolver.h"
#include "roc_netio/tcp_connection_port.h"
#include "roc_netio/tcp_server_port.h"
#include "roc_netio/udp_port.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace netio {

//! Network event loop thread.
//! @remarks
//!  This class is a task-based facade for the whole roc_netio module.
class NetworkLoop : private ITerminateHandler,
                    private ICloseHandler,
                    private IResolverRequestHandler,
                    private core::Thread {
public:
    //! Opaque port handle.
    typedef struct PortHandle* PortHandle;

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Add UDP datagram sender/receiver port.
        class AddUdpPort : public NetworkTask {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Updates @p config with the actual bind address.
            AddUdpPort(UdpConfig& config);

            //! Get created port handle.
            //! @pre
            //!  Should be called only after success() is true.
            PortHandle get_handle() const;

        private:
            friend class NetworkLoop;

            UdpConfig* config_;
        };

        //! Start sending on UDP port.
        class StartUdpSend : public NetworkTask {
        public:
            //! Set task parameters.
            //! @remarks
            //!  get_outbound_writer() returns a writer for packets to be send. It may be
            //!  used from another thread. It doesn't block the caller
            StartUdpSend(PortHandle handle);

            //! Get created writer for outbound packets.
            //! @pre
            //!  Should be called only after success() is true.
            packet::IWriter& get_outbound_writer() const;

        private:
            friend class NetworkLoop;

            packet::IWriter* outbound_writer_;
        };

        //! Start receiving on UDP port.
        class StartUdpRecv : public NetworkTask {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Received packets will be passed to @p inbound_writer.
            //!  It is invoked from network thread. It should not block the caller.
            StartUdpRecv(PortHandle handle, packet::IWriter& inbound_writer);

        private:
            friend class NetworkLoop;

            packet::IWriter* inbound_writer_;
        };

        //! Add TCP server port.
        class AddTcpServerPort : public NetworkTask {
        public:
            //! Set task parameters.
            //! @remarks
            //!  - Updates @p config with the actual bind address.
            //!  - Listens for incoming connections and passes new connections
            //!    to @p conn_acceptor. It should return handler that will be
            //!    notified when connection state changes.
            AddTcpServerPort(TcpServerConfig& config, IConnAcceptor& conn_acceptor);

            //! Get created port handle.
            //! @pre
            //!  Should be called only after success() is true.
            PortHandle get_handle() const;

        private:
            friend class NetworkLoop;

            TcpServerConfig* config_;
            IConnAcceptor* conn_acceptor_;
        };

        //! Add TCP client port.
        class AddTcpClientPort : public NetworkTask {
        public:
            //! Set task parameters.
            //! @remarks
            //!  - Updates @p config with the actual bind address.
            //!  - Notofies @p conn_handler when connection state changes.
            AddTcpClientPort(TcpClientConfig& config, IConnHandler& conn_handler);

            //! Get created port handle.
            //! @pre
            //!  Should be called only after success() is true.
            PortHandle get_handle() const;

        private:
            friend class NetworkLoop;

            TcpClientConfig* config_;
            IConnHandler* conn_handler_;
        };

        //! Remove port.
        class RemovePort : public NetworkTask {
        public:
            //! Set task parameters.
            RemovePort(PortHandle handle);

        private:
            friend class NetworkLoop;
        };

        //! Resolve endpoint address.
        class ResolveEndpointAddress : public NetworkTask {
        public:
            //! Set task parameters.
            //! @remarks
            //!  Gets endpoint hostname, resolves it, and writes the resolved IP address
            //!  and the port from the endpoint to the resulting SocketAddr.
            ResolveEndpointAddress(const address::NetworkUri& endpoint_uri);

            //! Get resolved address.
            //! @pre
            //!  Should be called only after success() is true.
            const address::SocketAddr& get_address() const;

        private:
            friend class NetworkLoop;

            ResolverRequest resolve_req_;
        };
    };

    //! Initialize.
    //! @remarks
    //!  Start background thread if the object was successfully constructed.
    NetworkLoop(core::IPool& packet_pool, core::IPool& buffer_pool, core::IArena& arena);

    //! Destroy. Stop all receivers and senders.
    //! @remarks
    //!  Wait until background thread finishes.
    virtual ~NetworkLoop();

    //! Check if control loop was successfully constructed.
    status::StatusCode init_status() const;

    //! Get number of receiver and sender ports.
    size_t num_ports() const;

    //! Enqueue a task for asynchronous execution and return.
    //! The task should not be destroyed until the callback is called.
    //! The @p completer will be invoked on event loop thread after the
    //! task completes.
    void schedule(NetworkTask& task, INetworkTaskCompleter& completer);

    //! Enqueue a task for asynchronous execution and wait for its completion.
    //! The task should not be destroyed until this method returns.
    //! Should not be called from schedule() callback.
    //! @returns
    //!  true if the task succeeded or false if it failed.
    ROC_ATTR_NODISCARD bool schedule_and_wait(NetworkTask& task);

private:
    static void task_sem_cb_(uv_async_t* handle);
    static void stop_sem_cb_(uv_async_t* handle);

    virtual void handle_terminate_completed(IConn&, void*);
    virtual void handle_close_completed(BasicPort&, void*);
    virtual void handle_resolved(ResolverRequest& req);

    virtual void run();

    void process_pending_tasks_();
    void finish_task_(NetworkTask&);

    void async_terminate_conn_port_(const core::SharedPtr<TcpConnectionPort>& port,
                                    NetworkTask* task);
    AsyncOperationStatus async_close_port_(const core::SharedPtr<BasicPort>& port,
                                           NetworkTask* task);
    void finish_closing_port_(const core::SharedPtr<BasicPort>& port, NetworkTask* task);

    void update_num_ports_();

    void close_all_sems_();
    void close_all_ports_();

    void task_add_udp_port_(NetworkTask&);
    void task_start_udp_send_(NetworkTask&);
    void task_start_udp_recv_(NetworkTask&);
    void task_add_tcp_server_(NetworkTask&);
    void task_add_tcp_client_(NetworkTask&);
    void task_remove_port_(NetworkTask&);
    void task_resolve_endpoint_address_(NetworkTask&);

    packet::PacketFactory packet_factory_;
    core::IArena& arena_;

    bool started_;

    uv_loop_t loop_;
    bool loop_initialized_;

    uv_async_t stop_sem_;
    bool stop_sem_initialized_;

    uv_async_t task_sem_;
    bool task_sem_initialized_;

    core::MpscQueue<NetworkTask, core::NoOwnership> pending_tasks_;

    Resolver resolver_;

    core::List<BasicPort> open_ports_;
    core::List<BasicPort> closing_ports_;

    core::Atomic<int> num_open_ports_;

    status::StatusCode init_status_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_NETWORK_LOOP_H_
