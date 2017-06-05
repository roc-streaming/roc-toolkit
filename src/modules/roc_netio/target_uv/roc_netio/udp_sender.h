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

#include "roc_config/config.h"

#include "roc_core/array.h"
#include "roc_core/atomic.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/spin_mutex.h"

#include "roc_datagram/address.h"
#include "roc_datagram/idatagram_writer.h"

#include "roc_netio/udp_datagram.h"

namespace roc {
namespace netio {

//! UDP sender.
class UDPSender : public datagram::IDatagramWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    UDPSender();

    //! Destroy.
    ~UDPSender();

    //! Attach to event loop.
    void attach(uv_loop_t&, uv_async_t& eof);

    //! Detach from event loop.
    void detach(uv_loop_t&);

    //! Add sending port.
    bool add_port(const datagram::Address&);

    //! Write datagram.
    virtual void write(const datagram::IDatagramPtr&);

private:
    enum { MaxPorts = ROC_CONFIG_MAX_PORTS };

    struct Port : core::NonCopyable<> {
        uv_udp_t handle;
        datagram::Address address;
    };

    static void async_cb_(uv_async_t* handle);
    static void send_cb_(uv_udp_send_t* req, int status);

    bool open_port_(Port& port);
    void close_port_(Port& port);
    Port* find_port_(const datagram::Address& address);

    void check_eof_();

    UDPDatagramPtr read_();

    core::Array<Port, MaxPorts> ports_;

    uv_loop_t* loop_;
    uv_async_t async_;
    uv_async_t* eof_;

    core::List<UDPDatagram> list_;
    core::SpinMutex mutex_;

    core::Atomic terminate_;
    core::Atomic pending_;

    unsigned number_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_SENDER_H_
