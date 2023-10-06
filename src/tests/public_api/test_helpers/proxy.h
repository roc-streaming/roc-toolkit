/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PUBLIC_API_TEST_HELPERS_PROXY_H_
#define ROC_PUBLIC_API_TEST_HELPERS_PROXY_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_address/socket_addr.h"
#include "roc_core/heap_arena.h"
#include "roc_netio/network_loop.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"

#include "roc/endpoint.h"

namespace roc {
namespace api {
namespace test {

class Proxy : private packet::IWriter {
public:
    Proxy(const roc_endpoint* receiver_source_endp,
          const roc_endpoint* receiver_repair_endp,
          size_t n_source_packets,
          size_t n_repair_packets,
          core::HeapArena& arena,
          packet::PacketFactory& packet_factory,
          core::BufferFactory<uint8_t>& byte_buffer_factory)
        : net_loop_(packet_factory, byte_buffer_factory, arena)
        , n_source_packets_(n_source_packets)
        , n_repair_packets_(n_repair_packets)
        , pos_(0) {
        CHECK(net_loop_.is_valid());

        roc_protocol source_proto;
        CHECK(roc_endpoint_get_protocol(receiver_source_endp, &source_proto) == 0);

        roc_protocol repair_proto;
        CHECK(roc_endpoint_get_protocol(receiver_repair_endp, &repair_proto) == 0);

        int source_port = 0;
        CHECK(roc_endpoint_get_port(receiver_source_endp, &source_port) == 0);

        int repair_port = 0;
        CHECK(roc_endpoint_get_port(receiver_repair_endp, &repair_port) == 0);

        CHECK(receiver_source_endp_.set_host_port(address::Family_IPv4, "127.0.0.1",
                                                  source_port));
        CHECK(receiver_repair_endp_.set_host_port(address::Family_IPv4, "127.0.0.1",
                                                  repair_port));

        CHECK(send_config_.bind_address.set_host_port(address::Family_IPv4, "127.0.0.1",
                                                      0));

        CHECK(recv_source_config_.bind_address.set_host_port(address::Family_IPv4,
                                                             "127.0.0.1", 0));
        CHECK(recv_repair_config_.bind_address.set_host_port(address::Family_IPv4,
                                                             "127.0.0.1", 0));

        netio::NetworkLoop::PortHandle send_port = NULL;

        {
            netio::NetworkLoop::Tasks::AddUdpSenderPort task(send_config_);
            CHECK(net_loop_.schedule_and_wait(task));

            send_port = task.get_handle();
            CHECK(send_port);

            writer_ = task.get_writer();
            CHECK(writer_);
        }

        {
            netio::NetworkLoop::Tasks::AddUdpReceiverPort task(recv_source_config_,
                                                               *this);
            CHECK(net_loop_.schedule_and_wait(task));
        }

        {
            netio::NetworkLoop::Tasks::AddUdpReceiverPort task(recv_repair_config_,
                                                               *this);
            CHECK(net_loop_.schedule_and_wait(task));
        }

        CHECK(roc_endpoint_allocate(&input_source_endp_) == 0);
        CHECK(roc_endpoint_set_protocol(input_source_endp_, source_proto) == 0);
        CHECK(roc_endpoint_set_host(input_source_endp_, "127.0.0.1") == 0);
        CHECK(roc_endpoint_set_port(input_source_endp_,
                                    recv_source_config_.bind_address.port())
              == 0);

        CHECK(roc_endpoint_allocate(&input_repair_endp_) == 0);
        CHECK(roc_endpoint_set_protocol(input_repair_endp_, repair_proto) == 0);
        CHECK(roc_endpoint_set_host(input_repair_endp_, "127.0.0.1") == 0);
        CHECK(roc_endpoint_set_port(input_repair_endp_,
                                    recv_repair_config_.bind_address.port())
              == 0);
    }

    ~Proxy() {
        CHECK(roc_endpoint_deallocate(input_source_endp_) == 0);
        CHECK(roc_endpoint_deallocate(input_repair_endp_) == 0);
    }

    const roc_endpoint* source_endpoint() const {
        return input_source_endp_;
    }

    const roc_endpoint* repair_endpoint() const {
        return input_repair_endp_;
    }

private:
    virtual void write(const packet::PacketPtr& pp) {
        pp->udp()->src_addr = send_config_.bind_address;

        if (pp->udp()->dst_addr == recv_source_config_.bind_address) {
            pp->udp()->dst_addr = receiver_source_endp_;
            source_queue_.write(pp);
        } else {
            pp->udp()->dst_addr = receiver_repair_endp_;
            repair_queue_.write(pp);
        }

        for (;;) {
            const size_t block_pos = pos_ % (n_source_packets_ + n_repair_packets_);

            if (block_pos < n_source_packets_) {
                if (!send_packet_(source_queue_, block_pos == 1)) {
                    return;
                }
            } else {
                if (!send_packet_(repair_queue_, false)) {
                    return;
                }
            }
        }
    }

    bool send_packet_(packet::IReader& reader, bool drop) {
        packet::PacketPtr pp;
        const status::StatusCode code = reader.read(pp);
        if (code != status::StatusOK) {
            CHECK(!pp);
            return false;
        }
        UNSIGNED_LONGS_EQUAL(status::StatusOK, code);
        CHECK(pp);

        pos_++;
        if (!drop) {
            writer_->write(pp);
        }
        return true;
    }

    netio::UdpSenderConfig send_config_;

    roc_endpoint* input_source_endp_;
    roc_endpoint* input_repair_endp_;

    netio::UdpReceiverConfig recv_source_config_;
    netio::UdpReceiverConfig recv_repair_config_;

    address::SocketAddr receiver_source_endp_;
    address::SocketAddr receiver_repair_endp_;

    packet::Queue source_queue_;
    packet::Queue repair_queue_;

    packet::IWriter* writer_;

    netio::NetworkLoop net_loop_;

    const size_t n_source_packets_;
    const size_t n_repair_packets_;

    size_t pos_;
};

} // namespace test
} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_TEST_HELPERS_PROXY_H_
