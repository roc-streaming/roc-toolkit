/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/conn_expectation.h"
#include "test_helpers/conn_reader.h"
#include "test_helpers/conn_writer.h"
#include "test_helpers/mock_conn_acceptor.h"
#include "test_helpers/mock_conn_handler.h"

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_netio/network_loop.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500, TotalBytes = 107701 };

core::HeapArena arena;
core::SlabPool<core::Buffer> buffer_pool("buffer_pool", arena, MaxBufSize);
core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);

TcpServerConfig make_server_config(const char* ip, int port) {
    TcpServerConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port)
          || config.bind_address.set_host_port(address::Family_IPv6, ip, port));
    return config;
}

TcpClientConfig make_client_config(const char* local_ip,
                                   int local_port,
                                   const char* remote_ip,
                                   int remote_port) {
    TcpClientConfig config;
    CHECK(config.local_address.set_host_port(address::Family_IPv4, local_ip, local_port)
          || config.local_address.set_host_port(address::Family_IPv6, local_ip,
                                                local_port));
    CHECK(
        config.remote_address.set_host_port(address::Family_IPv4, remote_ip, remote_port)
        || config.remote_address.set_host_port(address::Family_IPv6, remote_ip,
                                               remote_port));
    return config;
}

NetworkLoop::PortHandle add_tcp_server(NetworkLoop& net_loop,
                                       TcpServerConfig& config,
                                       IConnAcceptor& conn_acceptor) {
    NetworkLoop::Tasks::AddTcpServerPort task(config, conn_acceptor);
    CHECK(!task.success());
    if (!net_loop.schedule_and_wait(task)) {
        CHECK(!task.success());
        return NULL;
    }
    CHECK(task.success());
    return task.get_handle();
}

NetworkLoop::PortHandle add_tcp_client(NetworkLoop& net_loop,
                                       TcpClientConfig& config,
                                       IConnHandler& conn_handler) {
    NetworkLoop::Tasks::AddTcpClientPort task(config, conn_handler);
    CHECK(!task.success());
    if (!net_loop.schedule_and_wait(task)) {
        CHECK(!task.success());
        return NULL;
    }
    CHECK(task.success());
    return task.get_handle();
}

void terminate_and_wait(test::MockConnHandler& handler,
                        IConn* conn,
                        test::ConnExpectation exp) {
    CHECK(conn);

    conn->async_terminate(Term_Normal);
    handler.wait_terminated(exp);
}

} // namespace

TEST_GROUP(tcp_io) {};

TEST(tcp_io, one_server_one_client_one_direction) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    CHECK(add_tcp_server(net_loop, server_config, acceptor));

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config, client_conn_handler));

    IConn* server_conn = server_conn_handler.wait_established();
    IConn* client_conn = client_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    test::ConnReader reader(client_conn_handler, *client_conn, TotalBytes);
    test::ConnWriter writer(server_conn_handler, *server_conn, TotalBytes);

    CHECK(reader.start());
    CHECK(writer.start());

    reader.join();
    writer.join();

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
}

TEST(tcp_io, one_server_one_client_both_directions) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    CHECK(add_tcp_server(net_loop, server_config, acceptor));

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config, client_conn_handler));

    IConn* server_conn = server_conn_handler.wait_established();
    IConn* client_conn = client_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    test::ConnReader client_reader(client_conn_handler, *client_conn, TotalBytes);
    test::ConnWriter client_writer(client_conn_handler, *client_conn, TotalBytes);

    test::ConnReader server_reader(server_conn_handler, *server_conn, TotalBytes);
    test::ConnWriter server_writer(server_conn_handler, *server_conn, TotalBytes);

    CHECK(client_reader.start());
    CHECK(server_reader.start());

    CHECK(client_writer.start());
    CHECK(server_writer.start());

    client_writer.join();
    server_writer.join();

    client_reader.join();
    server_reader.join();

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
}

TEST(tcp_io, one_server_one_client_separate_loops) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop client_net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, client_net_loop.init_status());

    NetworkLoop server_net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, server_net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    CHECK(add_tcp_server(server_net_loop, server_config, acceptor));

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    CHECK(add_tcp_client(client_net_loop, client_config, client_conn_handler));

    IConn* server_conn = server_conn_handler.wait_established();
    IConn* client_conn = client_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    test::ConnReader reader(client_conn_handler, *client_conn, TotalBytes);
    test::ConnWriter writer(server_conn_handler, *server_conn, TotalBytes);

    CHECK(reader.start());
    CHECK(writer.start());

    reader.join();
    writer.join();

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
}

TEST(tcp_io, one_server_many_clients) {
    test::MockConnHandler client_conn_handler1;
    test::MockConnHandler client_conn_handler2;

    test::MockConnHandler server_conn_handler1;
    test::MockConnHandler server_conn_handler2;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler1);
    acceptor.push_handler(server_conn_handler2);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    CHECK(add_tcp_server(net_loop, server_config, acceptor));

    TcpClientConfig client_config1 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config1, client_conn_handler1));

    IConn* server_conn1 = server_conn_handler1.wait_established();
    IConn* client_conn1 = client_conn_handler1.wait_established();

    POINTERS_EQUAL(server_conn1, acceptor.wait_added());

    TcpClientConfig client_config2 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config2, client_conn_handler2));

    IConn* server_conn2 = server_conn_handler2.wait_established();
    IConn* client_conn2 = client_conn_handler2.wait_established();

    POINTERS_EQUAL(server_conn2, acceptor.wait_added());

    test::ConnReader reader1(client_conn_handler1, *client_conn1, TotalBytes);
    test::ConnWriter writer1(server_conn_handler1, *server_conn1, TotalBytes);

    test::ConnReader reader2(client_conn_handler2, *client_conn2, TotalBytes);
    test::ConnWriter writer2(server_conn_handler2, *server_conn2, TotalBytes);

    CHECK(reader1.start());
    CHECK(writer1.start());

    CHECK(reader2.start());
    CHECK(writer2.start());

    reader1.join();
    writer1.join();

    reader2.join();
    writer2.join();

    terminate_and_wait(server_conn_handler1, server_conn1, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler1, client_conn1, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler1, acceptor.wait_removed());

    terminate_and_wait(server_conn_handler2, server_conn2, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler2, client_conn2, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler2, acceptor.wait_removed());
}

} // namespace netio
} // namespace roc
