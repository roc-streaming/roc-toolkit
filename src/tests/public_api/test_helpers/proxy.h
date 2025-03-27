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
#include "roc_core/atomic.h"
#include "roc_core/fast_random.h"
#include "roc_core/heap_arena.h"
#include "roc_core/thread.h"
#include "roc_netio/network_loop.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_status/status_code.h"

#include "roc/endpoint.h"

namespace roc {
namespace api {
namespace test {

class Proxy : public core::Thread, private packet::IWriter {
public:
    Proxy(const roc_endpoint* receiver_source_endp,
          const roc_endpoint* receiver_repair_endp,
          size_t n_source_packets,
          size_t n_repair_packets,
          unsigned flags)
        : packet_pool_("proxy_packet_pool", arena_)
        , buffer_pool_("proxy_buffer_pool", arena_, 2000)
        , queue_(packet::ConcurrentQueue::Blocking)
        , net_loop_(
              packet_pool_, buffer_pool_, netio::NetworkLoop::DEFAULT_PRIORITY, arena_)
        , n_source_packets_(n_source_packets)
        , n_repair_packets_(n_repair_packets)
        , flags_(flags)
        , pos_(0) {
        LONGS_EQUAL(status::StatusOK, net_loop_.init_status());

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

    size_t n_dropped_packets() const {
        return n_dropped_packets_;
    }

    void stop_and_join() {
        stopped_ = true;
        LONGS_EQUAL(status::StatusOK, queue_.write(NULL));
        join();
    }

private:
    virtual void run() {
        bool first_packet = true;

        for (;;) {
            packet::PacketPtr pp;
            const status::StatusCode code = queue_.read(pp, packet::ModeFetch);
            if (code == status::StatusDrain) {
                break;
            }
            CHECK(code == status::StatusOK);
            CHECK(pp);

            if (stopped_) {
                break;
            }

            if (first_packet) {
                first_packet = false;
                if (flags_ & test::FlagDeliveryDelay) {
                    delivery_delay_();
                }
            }

            if (flags_ & test::FlagDeliveryJitter) {
                delivery_jitter_();
            }

            LONGS_EQUAL(status::StatusOK, writer_->write(pp));
        }
    }

    virtual ROC_NODISCARD status::StatusCode write(const packet::PacketPtr& pp) {
        pp->udp()->src_addr = send_config_.bind_address;

        if (pp->udp()->dst_addr == recv_source_config_.bind_address) {
            pp->udp()->dst_addr = receiver_source_endp_;
            LONGS_EQUAL(status::StatusOK, source_queue_.write(pp));
        } else {
            pp->udp()->dst_addr = receiver_repair_endp_;
            LONGS_EQUAL(status::StatusOK, repair_queue_.write(pp));
        }

        for (;;) {
            const size_t block_pos = pos_ % (n_source_packets_ + n_repair_packets_);

            if (block_pos < n_source_packets_) {
                const bool drop_packet = (flags_ & FlagLoseSomePkts) && (block_pos == 1);

                if (!send_packet_(source_queue_, drop_packet)) {
                    break;
                }
            } else {
                const bool drop_packet = (flags_ & FlagLoseAllRepairPkts);

                if (!send_packet_(repair_queue_, drop_packet)) {
                    break;
                }
            }
        }

        return status::StatusOK;
    }

    bool send_packet_(packet::IReader& reader, bool drop) {
        packet::PacketPtr pp;
        status::StatusCode code = reader.read(pp, packet::ModeFetch);
        if (code != status::StatusOK) {
            LONGS_EQUAL(status::StatusDrain, code);
            CHECK(!pp);
            return false;
        }
        CHECK(pp);
        pos_++;
        if (drop) {
            n_dropped_packets_++;
        } else {
            LONGS_EQUAL(status::StatusOK, queue_.write(pp));
        }
        return true;
    }

    void delivery_delay_() {
        core::sleep_for(core::ClockMonotonic,
                        (core::nanoseconds_t)core::fast_random_range(
                            core::Millisecond * 2, core::Millisecond * 5));
    }

    void delivery_jitter_() {
        core::sleep_for(core::ClockMonotonic,
                        (core::nanoseconds_t)core::fast_random_range(
                            core::Microsecond * 1, core::Microsecond * 5));
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

    packet::FifoQueue source_queue_;
    packet::FifoQueue repair_queue_;

    packet::ConcurrentQueue queue_;
    packet::IWriter* writer_;

    netio::NetworkLoop net_loop_;

    const size_t n_source_packets_;
    const size_t n_repair_packets_;
    core::Atomic<size_t> n_dropped_packets_;

    const unsigned flags_;
    size_t pos_;

    core::Atomic<int> stopped_;
};

} // namespace test
} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_TEST_HELPERS_PROXY_H_
