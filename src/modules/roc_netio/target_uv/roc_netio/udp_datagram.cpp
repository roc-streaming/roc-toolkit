/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/udp_datagram.h"
#include "roc_core/helpers.h"

namespace roc {
namespace netio {

const datagram::DatagramType UDPDatagram::Type = "roc::netio::UDPDatagram";

UDPDatagram::UDPDatagram(core::IPool<UDPDatagram>& pool)
    : pool_(pool) {
}

void UDPDatagram::free() {
    pool_.destroy(*this);
}

UDPDatagram* UDPDatagram::container_of(uv_udp_send_t* req) {
    return ROC_CONTAINER_OF(req, UDPDatagram, request_);
}

uv_udp_send_t& UDPDatagram::request() {
    return request_;
}

datagram::DatagramType UDPDatagram::type() const {
    return UDPDatagram::Type;
}

const core::IByteBufferConstSlice& UDPDatagram::buffer() const {
    return buffer_;
}

void UDPDatagram::set_buffer(const core::IByteBufferConstSlice& buff) {
    buffer_ = buff;
}

const datagram::Address& UDPDatagram::sender() const {
    return sender_;
}

void UDPDatagram::set_sender(const datagram::Address& address) {
    sender_ = address;
}

const datagram::Address& UDPDatagram::receiver() const {
    return receiver_;
}

void UDPDatagram::set_receiver(const datagram::Address& address) {
    receiver_ = address;
}

} // namespace netio
} // namespace roc
