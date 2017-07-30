/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_uv/roc_netio/udp_sender.h
//! @brief UDP sender.

#ifndef ROC_NETIO_UDP_SENDER_H_
#define ROC_NETIO_UDP_SENDER_H_

#include <uv.h>

#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/mutex.h"
#include "roc_core/refcnt.h"
#include "roc_packet/address.h"
#include "roc_packet/iwriter.h"

namespace roc {
namespace netio {

//! UDP sender.
class UDPSender : public core::RefCnt<UDPSender>,
                  public core::ListNode,
                  public packet::IWriter {
public:
    //! Initialize.
    UDPSender(uv_loop_t& event_loop, core::IAllocator& allocator);

    //! Destroy.
    ~UDPSender();

    //! Start sender.
    //! @remarks
    //!  Should be called from the event loop thread.
    bool start(packet::Address& bind_address);

    //! Asynchronous stop.
    //! @remarks
    //!  Should be called from the event loop thread.
    void stop();

    //! Write packet.
    //! @remarks
    //!  May be called from any thread.
    virtual void write(const packet::PacketPtr&);

private:
    static void write_sem_cb_(uv_async_t* handle);
    static void send_cb_(uv_udp_send_t* req, int status);

    friend class core::RefCnt<UDPSender>;

    void destroy();

    packet::PacketPtr read_();
    void close_();

    core::IAllocator& allocator_;

    uv_loop_t& loop_;

    uv_async_t write_sem_;
    bool write_sem_initialized_;

    uv_udp_t handle_;
    bool handle_initialized_;

    packet::Address address_;

    core::List<packet::Packet> list_;
    core::Mutex mutex_;

    size_t pending_;
    bool stopped_;

    unsigned packet_counter_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_SENDER_H_
