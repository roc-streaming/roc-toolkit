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

#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/thread.h"
#include "roc_netio/udp_receiver.h"
#include "roc_netio/udp_sender.h"
#include "roc_packet/address.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

//! Network sender/receiver.
class Transceiver : public core::Thread {
public:
    //! Initialize.
    Transceiver(packet::PacketPool& packet_pool,
                core::BufferPool<uint8_t>& buffer_pool,
                core::IAllocator& allocator);

    virtual ~Transceiver();

    //! Check if trasceiver was successfully constructed.
    bool valid() const;

    //! Add UDP datagram receiver.
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
    //!
    //! @pre
    //!  Should be called before start().
    bool add_udp_receiver(packet::Address& bind_address, packet::IWriter& writer);

    //! Add UDP datagram sender.
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
    //!
    //! @pre
    //!  Should be called before start().
    packet::IWriter* add_udp_sender(packet::Address& bind_address);

    //! Asynchronous stop.
    //! @remarks
    //!  Asynchronously stops all receivers and senders. May be called from
    //!  any thread. Use join() to wait until the stop operation finishes.
    void stop();

private:
    static void stop_sem_cb_(uv_async_t* handle);

    bool init_();
    void stop_io_();
    void close_sem_();

    virtual void run();

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;
    core::IAllocator& allocator_;

    uv_loop_t loop_;
    bool loop_initialized_;

    uv_async_t stop_sem_;
    bool stop_sem_initialized_;

    core::Atomic stopped_;
    bool valid_;

    core::List<UDPReceiver> receivers_;
    core::List<UDPSender> senders_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TRANSCEIVER_H_
