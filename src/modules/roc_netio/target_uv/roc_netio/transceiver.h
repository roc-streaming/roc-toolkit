/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_uv/roc_netio/transceiver.h
//! @brief Network sender/receiver.

#ifndef ROC_NETIO_TRANSCEIVER_H_
#define ROC_NETIO_TRANSCEIVER_H_

#include <uv.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/mutex.h"
#include "roc_core/semaphore.h"
#include "roc_core/thread.h"
#include "roc_netio/udp_receiver.h"
#include "roc_netio/udp_sender.h"
#include "roc_packet/address.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

//! Network sender/receiver.
class Transceiver : private core::Thread {
public:
    //! Initialize.
    Transceiver(packet::PacketPool& packet_pool,
                core::BufferPool<uint8_t>& buffer_pool,
                core::IAllocator& allocator);

    virtual ~Transceiver();

    //! Check if trasceiver was successfully constructed.
    bool valid() const;

    //! Start background thread.
    //! @remarks
    //!  Should be called once.
    void start();

    //! Asynchronous stop.
    //! @remarks
    //!  Asynchronously stops all receivers and senders. May be called from
    //!  any thread. Use join() to wait until the background thread finishes.
    void stop();

    //! Wait until background thread finishes.
    //! @remarks
    //!  Should be called once.
    void join();

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
    //!  true on success or false if error occured
    bool add_udp_receiver(packet::Address& bind_address, packet::IWriter& writer);

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
    //!  a new packet writer on success or null if error occured
    packet::IWriter* add_udp_sender(packet::Address& bind_address);

    //! Remove sender or receiver.
    void remove_port(packet::Address& bind_address);

private:
    struct Task : core::ListNode {
        bool (Transceiver::*fn)(Task&);

        packet::Address* address;
        packet::IWriter* writer;

        bool result;
        core::Semaphore done;

        Task()
            : fn(NULL)
            , address(NULL)
            , writer(NULL)
            , result(false)
            , done(0) {
        }
    };

    static void task_sem_cb_(uv_async_t* handle);
    static void stop_sem_cb_(uv_async_t* handle);

    virtual void run();

    void stop_();
    void close_sem_();

    void process_tasks_();
    void run_task_(Task&);

    bool add_udp_receiver_(Task&);
    bool add_udp_sender_(Task&);
    bool remove_port_(Task&);

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;
    core::IAllocator& allocator_;

    bool valid_;
    bool stopped_;

    uv_loop_t loop_;
    bool loop_initialized_;

    uv_async_t stop_sem_;
    bool stop_sem_initialized_;

    uv_async_t task_sem_;
    bool task_sem_initialized_;

    core::List<Task, core::NoOwnership> tasks_;

    core::List<UDPReceiver> receivers_;
    core::List<UDPSender> senders_;

    core::Mutex mutex_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TRANSCEIVER_H_
