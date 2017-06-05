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
#include "roc_core/noncopyable.h"
#include "roc_core/thread.h"

#include "roc_datagram/default_buffer_composer.h"
#include "roc_datagram/idatagram_composer.h"
#include "roc_datagram/idatagram_writer.h"

#include "roc_netio/udp_composer.h"
#include "roc_netio/udp_receiver.h"
#include "roc_netio/udp_sender.h"

namespace roc {
namespace netio {

//! Network sender/receiver.
class Transceiver : public core::Thread, public core::NonCopyable<> {
public:
    //! Initialize.
    Transceiver(
        core::IByteBufferComposer& buf_composer = datagram::default_buffer_composer(),
        core::IPool<UDPDatagram>& dgm_pool = core::HeapPool<UDPDatagram>::instance());

    ~Transceiver();

    //! Add UDP datagram receiver.
    //! @remarks
    //!  Datagrams received on @p address will be passed to @p writer.
    //! @note
    //!  Writer will be called from network thread.
    //! @pre
    //!  In current implementation, this method should be called before
    //!  starting thread using start().
    bool add_udp_receiver(const datagram::Address& address,
                          datagram::IDatagramWriter& writer);

    //! Add UDP datagram sender.
    //! @remarks
    //!  After this call, udp_sender() may be used to send datagrams with
    //!  @p address set as sender address.
    //! @pre
    //!  In current implementation, this method should be called before
    //!  starting thread using start().
    bool add_udp_sender(const datagram::Address& address);

    //! Get UDP datagram composer.
    //! @remarks
    //!  Datagrams passed to udp_sender() should be created using udp_composer().
    //! @note
    //!  Returned object may be used from any thread.
    datagram::IDatagramComposer& udp_composer();

    //! Get UDP datagram sender.
    //! @remarks
    //!  Datagrams should be created using udp_composer() and has sender address
    //!  previously registered with add_udp_sender().
    //! @note
    //!  Returned object may be used from any thread.
    datagram::IDatagramWriter& udp_sender();

    //! Stop thread.
    //! @remarks
    //!  May be called from any thread. After this call, subsequent join()
    //!  call will return soon.
    void stop();

private:
    static void async_cb_(uv_async_t* handle);

    virtual void run();

    void close_();

    uv_loop_t loop_;
    uv_async_t async_;

    UDPComposer udp_composer_;
    UDPReceiver udp_receiver_;
    UDPSender udp_sender_;

    core::Atomic stop_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TRANSCEIVER_H_
