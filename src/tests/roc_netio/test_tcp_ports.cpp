/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/cond.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/list_node.h"
#include "roc_core/mutex.h"
#include "roc_core/refcnt.h"
#include "roc_core/shared_ptr.h"
#include "roc_netio/iconn_acceptor.h"
#include "roc_netio/iconn_notifier.h"
#include "roc_netio/tcp_conn.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

namespace {

address::SocketAddr make_address(const char* ip, int port) {
    address::SocketAddr addr;
    CHECK(addr.set_host_port_ipv4(ip, port));
    return addr;
}

class TestConnNotifier : public IConnNotifier {
public:
    TestConnNotifier()
        : cond_(mutex_)
        , connected_(false)
        , written_(false)
        , readable_(false) {
    }

    virtual void notify_connected(bool connected) {
        CHECK(connected);

        core::Mutex::Lock lock(mutex_);

        connected_ = true;
        cond_.broadcast();
    }

    virtual void notify_readable() {
        core::Mutex::Lock lock(mutex_);

        readable_ = true;
        cond_.broadcast();
    }

    virtual void notify_writable(bool written) {
        CHECK(written);

        core::Mutex::Lock lock(mutex_);

        written_ = true;
        cond_.broadcast();
    }

    void wait_connected() {
        core::Mutex::Lock lock(mutex_);

        while (!connected_) {
            cond_.wait();
        }
    }

    void wait_written() {
        core::Mutex::Lock lock(mutex_);

        while (!written_) {
            cond_.wait();
        }
    }

    void wait_readable() {
        core::Mutex::Lock lock(mutex_);

        while (!readable_) {
            cond_.wait();
        }
    }

private:
    core::Mutex mutex_;
    core::Cond cond_;

    bool connected_;
    bool written_;
    bool readable_;
};

class TestConnAcceptor : public IConnAcceptor {
public:
    explicit TestConnAcceptor(core::IAllocator& a)
        : allocator_(a) {
    }

    size_t num_connections() const {
        core::Mutex::Lock lock(mutex_);

        return holders_.size();
    }

    virtual IConnNotifier* accept(TCPConn& conn) {
        core::Mutex::Lock lock(mutex_);

        core::SharedPtr<TCPConnHolder> holder(new (allocator_)
                                                  TCPConnHolder(conn, allocator_));
        holders_.push_back(*holder);

        return &holder->conn_notifier();
    }

    TCPConn* get_connection(const address::SocketAddr& serv_addr,
                            const address::SocketAddr& client_addr) {
        core::Mutex::Lock lock(mutex_);

        core::SharedPtr<TCPConnHolder> holder;

        for (holder = holders_.front(); holder; holder = holders_.nextof(*holder)) {
            if (holder->connection().destination_address() == serv_addr
                && holder->connection().address() == client_addr) {
                return &holder->connection();
            }
        }

        return NULL;
    }

private:
    class TCPConnHolder : public core::RefCnt<TCPConnHolder>, public core::ListNode {
    public:
        TCPConnHolder(TCPConn& conn, core::IAllocator& allocator)
            : conn_(conn)
            , allocator_(allocator) {
        }

        TCPConn& connection() {
            return conn_;
        }

        TestConnNotifier& conn_notifier() {
            return conn_notifier_;
        }

    private:
        friend class core::RefCnt<TCPConnHolder>;

        void destroy() {
            allocator_.destroy(*this);
        }

        TestConnNotifier conn_notifier_;

        TCPConn& conn_;
        core::IAllocator& allocator_;
    };

    core::Mutex mutex_;
    core::IAllocator& allocator_;
    core::List<TCPConnHolder> holders_;
};

enum { MaxBufSize = 500 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

} // namespace

TEST_GROUP(tcp_ports) {};

TEST(tcp_ports, no_ports) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());
}

TEST(tcp_ports, tcp_add_server_no_remove) {
    TestConnAcceptor conn_acceptor(allocator);

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(addr, conn_acceptor));
}

TEST(tcp_ports, tcp_add_remove_server) {
    TestConnAcceptor conn_acceptor(allocator);

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(addr, conn_acceptor));
    trx.remove_port(addr);
}

TEST(tcp_ports, tcp_add_client_server_no_remove) {
    TestConnAcceptor conn_acceptor(allocator);
    TestConnNotifier conn_notifier;

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr server_address = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(server_address, conn_acceptor));

    CHECK(trx.add_tcp_client(server_address, conn_notifier));
    conn_notifier.wait_connected();
}

TEST(tcp_ports, tcp_remove_server) {
    TestConnAcceptor conn_acceptor(allocator);
    TestConnNotifier conn_notifier;

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr server_address = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(server_address, conn_acceptor));

    CHECK(trx.add_tcp_client(server_address, conn_notifier));
    conn_notifier.wait_connected();

    trx.remove_port(server_address);
}

TEST(tcp_ports, tcp_remove_client) {
    TestConnAcceptor conn_acceptor(allocator);
    TestConnNotifier conn_notifier;

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr server_address = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(server_address, conn_acceptor));

    TCPConn* conn = trx.add_tcp_client(server_address, conn_notifier);
    CHECK(conn);

    trx.remove_port(conn->address());
}

TEST(tcp_ports, tcp_single_server_multiple_clients) {
    TestConnAcceptor conn_acceptor(allocator);

    TestConnNotifier conn_notifier1;
    TestConnNotifier conn_notifier2;

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr server_address = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(server_address, conn_acceptor));

    CHECK(trx.add_tcp_client(server_address, conn_notifier1));
    CHECK(trx.add_tcp_client(server_address, conn_notifier2));

    conn_notifier1.wait_connected();
    conn_notifier2.wait_connected();
}

TEST(tcp_ports, tcp_add_client_no_server) {
    /* TestConnAcceptor conn_acceptor(allocator); */
    /* TestConnNotifier conn_notifier; */

    /* Transceiver trx(packet_pool, buffer_pool, allocator); */
    /* CHECK(trx.valid()); */

    /* address::SocketAddr server_address = make_address("0.0.0.0", 0); */

    /* CHECK(!trx.add_tcp_client(server_address, conn_notifier)); */
}

TEST(tcp_ports, tcp_failed_to_accept) {
    /* TestConnAcceptor conn_acceptor(allocator); */
    /* TestConnNotifier conn_notifier; */

    /* Transceiver trx(packet_pool, buffer_pool, allocator); */
    /* CHECK(trx.valid()); */

    /* address::SocketAddr server_address = make_address("0.0.0.0", 0); */

    /* conn_acceptor.disable(); */

    /* CHECK(trx.add_tcp_server(server_address, conn_acceptor)); */
    /* CHECK(trx.add_tcp_client(server_address, conn_notifier)); */
}

TEST(tcp_ports, tcp_add_client_wait_connected) {
    TestConnAcceptor conn_acceptor(allocator);
    TestConnNotifier conn_notifier;

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr server_address = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(server_address, conn_acceptor));

    TCPConn* conn = trx.add_tcp_client(server_address, conn_notifier);
    CHECK(conn);

    conn_notifier.wait_connected();
    CHECK(conn_acceptor.num_connections() == 1);

    CHECK(conn->connected());
    CHECK(conn->address() != server_address);
    CHECK(conn->destination_address() == server_address);
}

TEST(tcp_ports, tcp_write_data) {
    TestConnAcceptor conn_acceptor(allocator);
    TestConnNotifier conn_notifier;

    Transceiver trx(packet_pool, buffer_pool, allocator);
    CHECK(trx.valid());

    address::SocketAddr server_address = make_address("0.0.0.0", 0);

    CHECK(trx.add_tcp_server(server_address, conn_acceptor));

    TCPConn* client_conn = trx.add_tcp_client(server_address, conn_notifier);
    CHECK(client_conn);

    conn_notifier.wait_connected();

    CHECK(client_conn->write("foo", strlen("foo")));
    conn_notifier.wait_written();

    TCPConn* serv_conn = conn_acceptor.get_connection(client_conn->destination_address(),
                                                      client_conn->address());
    CHECK(serv_conn);

    CHECK(serv_conn->write("bar", strlen("bar")));
    conn_notifier.wait_readable();

    char recv_resp[strlen("bar")];
    CHECK(client_conn->read(recv_resp, sizeof(recv_resp)) == sizeof(recv_resp));
    CHECK(memcmp("bar", recv_resp, strlen("bar")) == 0);
}

} // namespace netio
} // namespace roc
