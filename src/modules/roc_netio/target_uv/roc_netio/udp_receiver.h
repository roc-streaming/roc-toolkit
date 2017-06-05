/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_uv/roc_netio/udp_receiver.h
//! @brief UDP receiver.

#ifndef ROC_NETIO_UDP_RECEIVER_H_
#define ROC_NETIO_UDP_RECEIVER_H_

#include <uv.h>

#include "roc_config/config.h"

#include "roc_core/array.h"
#include "roc_core/byte_buffer.h"
#include "roc_core/noncopyable.h"

#include "roc_datagram/address.h"
#include "roc_datagram/idatagram_writer.h"

#include "roc_netio/udp_composer.h"
#include "roc_netio/udp_datagram.h"

namespace roc {
namespace netio {

//! UDP receiver.
class UDPReceiver : public core::NonCopyable<> {
public:
    //! Initialize.
    UDPReceiver(core::IByteBufferComposer& buf_composer, UDPComposer& dgm_composer);

    //! Destroy.
    ~UDPReceiver();

    //! Attach to event loop.
    void attach(uv_loop_t&);

    //! Detach from event loop.
    void detach(uv_loop_t&);

    //! Add receiving port.
    bool add_port(const datagram::Address&, datagram::IDatagramWriter&);

private:
    enum { MaxPorts = ROC_CONFIG_MAX_PORTS };

    struct Port : core::NonCopyable<> {
        uv_udp_t handle;

        datagram::Address address;
        datagram::IDatagramWriter* writer;

        Port()
            : writer(NULL) {
        }
    };

    static void alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf);
    static void recv_cb_(uv_udp_t* handle,
                         ssize_t nread,
                         const uv_buf_t* buf,
                         const sockaddr* addr,
                         unsigned flags);

    bool open_port_(Port& port);
    void close_port_(Port& port);

    core::Array<Port, MaxPorts> ports_;
    uv_loop_t* loop_;

    core::IByteBufferComposer& buf_composer_;
    UDPComposer& dgm_composer_;

    unsigned number_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_RECEIVER_H_
