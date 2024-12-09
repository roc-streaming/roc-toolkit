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
#include "roc_status/status_code.h"

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
          size_t n_control_packets) // Added control packets count
        : packet_pool_("proxy_packet_pool", arena_)
        , buffer_pool_("proxy_buffer_pool", arena_, 2000)
        , net_loop_(packet_pool_, buffer_pool_, arena_)
        , n_source_packets_(n_source_packets)
        , n_repair_packets_(n_repair_packets)
        , n_control_packets_(n_control_packets) // Initialize control packets count
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


        // Control endpoint setup
        if (receiver_control_endp) {
            roc_protocol control_proto;
            CHECK(roc_endpoint_get_protocol(receiver_control_endp, &control_proto) == 0);

            int control_port = 0;
            CHECK(roc_endpoint_get_port(receiver_control_endp, &control_port) == 0);

            CHECK(receiver_control_endp_.set_host_port(address::Family_IPv4, "127.0.0.1", control_port));

            CHECK(recv_control_config_.bind_address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));

            netio::NetworkLoop::Tasks::AddUdpPort add_task(recv_control_config_);
            CHECK(net_loop_.schedule_and_wait(add_task));

            netio::NetworkLoop::Tasks::StartUdpRecv recv_task(add_task.get_handle(), *this);
            CHECK(net_loop_.schedule_and_wait(recv_task));
        }

        netio::NetworkLoop::PortHandle send_port = NULL;

        {
            netio::NetworkLoop::Tasks::AddUdpPort add_task(send_config_);
            CHECK(net_loop_.schedule_and_wait(add_task));

            send_port = add_task.get_handle();
            CHECK(send_port);

            netio::NetworkLoop::Tasks::StartUdpSend send_task(add_task.get_handle());
            CHECK(net_loop_.schedule_and_wait(send_task));
            writer_ = &send_task.get_outbound_writer();
        }

        {
            netio::NetworkLoop::Tasks::AddUdpPort add_task(recv_source_config_);
            CHECK(net_loop_.schedule_and_wait(add_task));

            netio::NetworkLoop::Tasks::StartUdpRecv recv_task(add_task.get_handle(),
                                                              *this);
            CHECK(net_loop_.schedule_and_wait(recv_task));
        }

        {
            netio::NetworkLoop::Tasks::AddUdpPort add_task(recv_repair_config_);
            CHECK(net_loop_.schedule_and_wait(add_task));

            netio::NetworkLoop::Tasks::StartUdpRecv recv_task(add_task.get_handle(),
                                                              *this);
            CHECK(net_loop_.schedule_and_wait(recv_task));
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

        if (receiver_control_endp) {
            CHECK(roc_endpoint_allocate(&input_control_endp_) == 0);
            CHECK(roc_endpoint_set_protocol(input_control_endp_, control_proto) == 0);
            CHECK(roc_endpoint_set_host(input_control_endp_, "127.0.0.1") == 0);
            CHECK(roc_endpoint_set_port(input_control_endp_,
                                        recv_control_config_.bind_address.port())
                  == 0);
        }
    }

    ~Proxy() {
        CHECK(roc_endpoint_deallocate(input_source_endp_) == 0);
        CHECK(roc_endpoint_deallocate(input_repair_endp_) == 0);
        if (input_control_endp_) {
            CHECK(roc_endpoint_deallocate(input_control_endp_) == 0);
        }
    }

    const roc_endpoint* source_endpoint() const {
        return input_source_endp_;
    }

    const roc_endpoint* repair_endpoint() const {
        return input_repair_endp_;
    }

     const roc_endpoint* control_endpoint() const {
        return input_control_endp_;
    }

private:
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr& pp) {
        pp->udp()->src_addr = send_config_.bind_address;

        if (pp->udp()->dst_addr == recv_source_config_.bind_address) {
            pp->udp()->dst_addr = receiver_source_endp_;
            UNSIGNED_LONGS_EQUAL(status::StatusOK, source_queue_.write(pp));
        } else if (pp->udp()->dst_addr == recv_repair_config_.bind_address) {
            pp->udp()->dst_addr = receiver_repair_endp_;
            UNSIGNED_LONGS_EQUAL(status::StatusOK, repair_queue_.write(pp));
        } else if (pp->udp()->dst_addr == recv_control_config_.bind_address) {
            pp->udp()->dst_addr = receiver_control_endp_;
            UNSIGNED_LONGS_EQUAL(status::StatusOK, control_queue_.write(pp));
        }

        for (;;) {
            const size_t block_pos = pos_ % (n_source_packets_ + n_repair_packets_);

            if (block_pos < n_source_packets_) {
                if (!send_packet_(source_queue_, block_pos == 1)) {
                    break;
                }
            } else if (block_pos < n_source_packets_ + n_repair_packets_) {
                if (!send_packet_(repair_queue_, false)) {
                    break;
                }
            } else {
                if (!send_packet_(control_queue_, false)) {
                    break;
                }
            }

        }

        return status::StatusOK;
    }

    bool send_packet_(packet::IReader& reader, bool drop) {
        packet::PacketPtr pp;
        status::StatusCode code = reader.read(pp);
        if (code != status::StatusOK) {
            UNSIGNED_LONGS_EQUAL(status::StatusNoData, code);
            CHECK(!pp);
            return false;
        }
        CHECK(pp);
        pos_++;
        if (!drop) {
            UNSIGNED_LONGS_EQUAL(status::StatusOK, writer_->write(pp));
        }
        return true;
    }

    core::HeapArena arena_;

    core::SlabPool<packet::Packet> packet_pool_;
    core::SlabPool<core::Buffer> buffer_pool_;

    netio::UdpConfig send_config_;
    netio::UdpConfig recv_source_config_;
    netio::UdpConfig recv_repair_config_;

    roc_endpoint* input_source_endp_;
    roc_endpoint* input_repair_endp_;

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
