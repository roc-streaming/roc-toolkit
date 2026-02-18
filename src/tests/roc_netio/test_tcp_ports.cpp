/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/conn_expectation.h"
#include "test_helpers/mock_conn_acceptor.h"
#include "test_helpers/mock_conn_handler.h"

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_netio/network_loop.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapArena arena;
core::SlabPool<core::Buffer> buffer_pool("buffer_pool", arena, MaxBufSize);
core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);

address::SocketAddr make_address(const char* ip, int port) {
    address::SocketAddr address;
    CHECK(address.set_host_port(address::Family_IPv4, ip, port)
          || address.set_host_port(address::Family_IPv6, ip, port));
    return address;
}

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

void remove_port(NetworkLoop& net_loop, NetworkLoop::PortHandle handle) {
    NetworkLoop::Tasks::RemovePort task(handle);
    CHECK(!task.success());
    CHECK(net_loop.schedule_and_wait(task));
    CHECK(task.success());
}

void expect_local_remote(IConn* conn,
                         const address::SocketAddr& local_address,
                         const address::SocketAddr& remote_address) {
    CHECK(conn);

    STRCMP_EQUAL(address::socket_addr_to_str(local_address).c_str(),
                 address::socket_addr_to_str(conn->local_address()).c_str());

    STRCMP_EQUAL(address::socket_addr_to_str(remote_address).c_str(),
                 address::socket_addr_to_str(conn->remote_address()).c_str());
}

void wait_writable_readable(test::MockConnHandler& handler,
                            IConn* conn,
                            bool writable,
                            bool readable) {
    CHECK(conn);
    if (writable) {
        handler.wait_writable();
        CHECK(conn->is_writable());
    } else {
        CHECK(!conn->is_writable());
    }
    if (readable) {
        handler.wait_readable();
        CHECK(conn->is_readable());
    } else {
        CHECK(!conn->is_readable());
    }
}

void expect_write_error(IConn* conn, SocketError err) {
    CHECK(conn);
    char buf[1] = {};
    LONGS_EQUAL(err, conn->try_write(buf, sizeof(buf)));
}

void expect_read_error(IConn* conn, SocketError err) {
    CHECK(conn);
    char buf[1] = {};
    LONGS_EQUAL(err, conn->try_read(buf, sizeof(buf)));
}

void terminate_and_wait(test::MockConnHandler& handler,
                        IConn* conn,
                        test::ConnExpectation exp) {
    CHECK(conn);

    conn->async_terminate(Term_Normal);
    handler.wait_terminated(exp);
}

} // namespace

TEST_GROUP(tcp_ports) {};

TEST(tcp_ports, no_ports) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(tcp_ports, add_anyaddr) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);
    CHECK(server_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    TcpClientConfig client_config =
        make_client_config("0.0.0.0", 0, "127.0.0.1", server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop, client_config, client_conn_handler);
    CHECK(client_handle);
    CHECK(client_config.local_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    IConn* server_conn = server_conn_handler.wait_established();
    expect_local_remote(server_conn, server_config.bind_address,
                        make_address("127.0.0.1", client_config.local_address.port()));

    IConn* client_conn = client_conn_handler.wait_established();
    expect_local_remote(client_conn, client_config.local_address,
                        client_config.remote_address);

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
}

TEST(tcp_ports, add_localhost) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);
    CHECK(server_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop, client_config, client_conn_handler);
    CHECK(client_handle);
    CHECK(client_config.local_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    IConn* server_conn = server_conn_handler.wait_established();
    expect_local_remote(server_conn, server_config.bind_address,
                        client_config.local_address);

    IConn* client_conn = client_conn_handler.wait_established();
    expect_local_remote(client_conn, client_config.local_address,
                        client_config.remote_address);

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
}

TEST(tcp_ports, add_addrinuse) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop1(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop1.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop1.num_ports());

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop1, server_config, acceptor);
    CHECK(server_handle);
    CHECK(server_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop1.num_ports());

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop1, client_config, client_conn_handler);
    CHECK(client_handle);
    CHECK(client_config.local_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(2, net_loop1.num_ports());

    IConn* server_conn = server_conn_handler.wait_established();
    IConn* client_conn = client_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    NetworkLoop net_loop2(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop2.init_status());

    UNSIGNED_LONGS_EQUAL(0, net_loop2.num_ports());

    CHECK(!add_tcp_server(net_loop2, server_config, acceptor));
    CHECK(!add_tcp_client(net_loop2, client_config, client_conn_handler));

    UNSIGNED_LONGS_EQUAL(0, net_loop2.num_ports());

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
}

TEST(tcp_ports, add_remove) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);
    CHECK(server_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop, client_config, client_conn_handler);
    CHECK(client_handle);
    CHECK(client_config.local_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    IConn* server_conn = server_conn_handler.wait_established();
    IConn* client_conn = client_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    remove_port(net_loop, server_handle);
    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);
    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, client_handle);
    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(tcp_ports, add_remove_add) {
    test::MockConnAcceptor acceptor;

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);
    CHECK(server_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, server_handle);
    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    server_handle = add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());
}

TEST(tcp_ports, connect_one_server_one_client) {
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

    expect_local_remote(server_conn, server_config.bind_address,
                        client_config.local_address);
    expect_local_remote(client_conn, client_config.local_address,
                        client_config.remote_address);

    wait_writable_readable(server_conn_handler, server_conn, true, false);
    wait_writable_readable(client_conn_handler, client_conn, true, false);

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());
}

TEST(tcp_ports, connect_one_server_many_clients) {
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

    expect_local_remote(server_conn1, server_config.bind_address,
                        client_config1.local_address);
    expect_local_remote(client_conn1, client_config1.local_address,
                        client_config1.remote_address);

    wait_writable_readable(server_conn_handler1, server_conn1, true, false);
    wait_writable_readable(client_conn_handler1, client_conn1, true, false);

    TcpClientConfig client_config2 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config2, client_conn_handler2));

    IConn* server_conn2 = server_conn_handler2.wait_established();
    IConn* client_conn2 = client_conn_handler2.wait_established();

    POINTERS_EQUAL(server_conn2, acceptor.wait_added());

    expect_local_remote(server_conn2, server_config.bind_address,
                        client_config2.local_address);
    expect_local_remote(client_conn2, client_config2.local_address,
                        client_config2.remote_address);

    wait_writable_readable(server_conn_handler2, server_conn2, true, false);
    wait_writable_readable(client_conn_handler2, client_conn2, true, false);

    terminate_and_wait(server_conn_handler1, server_conn1, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler1, client_conn1, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler1, acceptor.wait_removed());

    terminate_and_wait(server_conn_handler2, server_conn2, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler2, client_conn2, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler2, acceptor.wait_removed());
}

TEST(tcp_ports, connect_one_server_many_clients_many_loops) {
    test::MockConnHandler client_conn_handler1;
    test::MockConnHandler client_conn_handler2;

    test::MockConnHandler server_conn_handler1;
    test::MockConnHandler server_conn_handler2;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler1);
    acceptor.push_handler(server_conn_handler2);

    NetworkLoop net_loop_client1(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop_client1.init_status());

    NetworkLoop net_loop_client2(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop_client2.init_status());

    NetworkLoop net_loop_server(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop_server.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    CHECK(add_tcp_server(net_loop_server, server_config, acceptor));

    TcpClientConfig client_config1 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop_client1, client_config1, client_conn_handler1));

    IConn* client_conn1 = client_conn_handler1.wait_established();
    IConn* server_conn1 = server_conn_handler1.wait_established();

    POINTERS_EQUAL(server_conn1, acceptor.wait_added());

    expect_local_remote(server_conn1, server_config.bind_address,
                        client_config1.local_address);
    expect_local_remote(client_conn1, client_config1.local_address,
                        client_config1.remote_address);

    wait_writable_readable(server_conn_handler1, server_conn1, true, false);
    wait_writable_readable(client_conn_handler1, client_conn1, true, false);

    TcpClientConfig client_config2 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop_client2, client_config2, client_conn_handler2));

    IConn* server_conn2 = server_conn_handler2.wait_established();
    IConn* client_conn2 = client_conn_handler2.wait_established();

    POINTERS_EQUAL(server_conn2, acceptor.wait_added());

    expect_local_remote(server_conn2, server_config.bind_address,
                        client_config2.local_address);
    expect_local_remote(client_conn2, client_config2.local_address,
                        client_config2.remote_address);

    wait_writable_readable(server_conn_handler2, server_conn2, true, false);
    wait_writable_readable(client_conn_handler2, client_conn2, true, false);

    terminate_and_wait(server_conn_handler1, server_conn1, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler1, client_conn1, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler1, acceptor.wait_removed());

    terminate_and_wait(server_conn_handler2, server_conn2, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler2, client_conn2, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler2, acceptor.wait_removed());
}

TEST(tcp_ports, connect_many_servers_many_clients) {
    test::MockConnHandler client_conn_handler1;
    test::MockConnHandler client_conn_handler2;

    test::MockConnHandler server_conn_handler1;
    test::MockConnHandler server_conn_handler2;

    test::MockConnAcceptor acceptor1;
    acceptor1.push_handler(server_conn_handler1);

    test::MockConnAcceptor acceptor2;
    acceptor2.push_handler(server_conn_handler2);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config1 = make_server_config("127.0.0.1", 0);
    CHECK(add_tcp_server(net_loop, server_config1, acceptor1));

    TcpServerConfig server_config2 = make_server_config("127.0.0.1", 0);
    CHECK(add_tcp_server(net_loop, server_config2, acceptor2));

    TcpClientConfig client_config1 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config1.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config1, client_conn_handler1));

    IConn* client_conn1 = client_conn_handler1.wait_established();
    IConn* server_conn1 = server_conn_handler1.wait_established();

    POINTERS_EQUAL(server_conn1, acceptor1.wait_added());

    expect_local_remote(server_conn1, server_config1.bind_address,
                        client_config1.local_address);
    expect_local_remote(client_conn1, client_config1.local_address,
                        client_config1.remote_address);

    wait_writable_readable(server_conn_handler1, server_conn1, true, false);
    wait_writable_readable(client_conn_handler1, client_conn1, true, false);

    TcpClientConfig client_config2 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config2.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config2, client_conn_handler2));

    IConn* client_conn2 = client_conn_handler2.wait_established();
    IConn* server_conn2 = server_conn_handler2.wait_established();

    POINTERS_EQUAL(server_conn2, acceptor2.wait_added());

    expect_local_remote(server_conn2, server_config2.bind_address,
                        client_config2.local_address);
    expect_local_remote(client_conn2, client_config2.local_address,
                        client_config2.remote_address);

    wait_writable_readable(server_conn_handler2, server_conn2, true, false);
    wait_writable_readable(client_conn_handler2, client_conn2, true, false);

    terminate_and_wait(server_conn_handler1, server_conn1, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler1, client_conn1, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler1, acceptor1.wait_removed());

    terminate_and_wait(server_conn_handler2, server_conn2, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler2, client_conn2, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler2, acceptor2.wait_removed());
}

TEST(tcp_ports, connect_many_servers_many_clients_many_loops) {
    test::MockConnHandler client_conn_handler1;
    test::MockConnHandler client_conn_handler2;

    test::MockConnHandler server_conn_handler1;
    test::MockConnHandler server_conn_handler2;

    test::MockConnAcceptor acceptor1;
    acceptor1.push_handler(server_conn_handler1);

    test::MockConnAcceptor acceptor2;
    acceptor2.push_handler(server_conn_handler2);

    NetworkLoop net_loop_client(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop_client.init_status());

    NetworkLoop net_loop_server(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop_server.init_status());

    TcpServerConfig server_config1 = make_server_config("127.0.0.1", 0);
    CHECK(add_tcp_server(net_loop_server, server_config1, acceptor1));

    TcpServerConfig server_config2 = make_server_config("127.0.0.1", 0);
    CHECK(add_tcp_server(net_loop_server, server_config2, acceptor2));

    TcpClientConfig client_config1 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config1.bind_address.port());

    CHECK(add_tcp_client(net_loop_client, client_config1, client_conn_handler1));

    IConn* client_conn1 = client_conn_handler1.wait_established();
    IConn* server_conn1 = server_conn_handler1.wait_established();

    POINTERS_EQUAL(server_conn1, acceptor1.wait_added());

    expect_local_remote(server_conn1, server_config1.bind_address,
                        client_config1.local_address);
    expect_local_remote(client_conn1, client_config1.local_address,
                        client_config1.remote_address);

    wait_writable_readable(server_conn_handler1, server_conn1, true, false);
    wait_writable_readable(client_conn_handler1, client_conn1, true, false);

    TcpClientConfig client_config2 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config2.bind_address.port());

    CHECK(add_tcp_client(net_loop_client, client_config2, client_conn_handler2));

    IConn* client_conn2 = client_conn_handler2.wait_established();
    IConn* server_conn2 = server_conn_handler2.wait_established();

    POINTERS_EQUAL(server_conn2, acceptor2.wait_added());

    expect_local_remote(server_conn2, server_config2.bind_address,
                        client_config2.local_address);
    expect_local_remote(client_conn2, client_config2.local_address,
                        client_config2.remote_address);

    wait_writable_readable(server_conn_handler2, server_conn2, true, false);
    wait_writable_readable(client_conn_handler2, client_conn2, true, false);

    terminate_and_wait(server_conn_handler1, server_conn1, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler1, client_conn1, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler1, acceptor1.wait_removed());

    terminate_and_wait(server_conn_handler2, server_conn2, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler2, client_conn2, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler2, acceptor2.wait_removed());
}

TEST(tcp_ports, connect_error) {
    test::MockConnHandler client_conn_handler1;
    test::MockConnHandler client_conn_handler2;
    test::MockConnHandler server_conn_handler1;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler1);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);
    CHECK(add_tcp_server(net_loop, server_config, acceptor));

    TcpClientConfig client_config1 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    CHECK(add_tcp_client(net_loop, client_config1, client_conn_handler1));

    IConn* client_conn1 = client_conn_handler1.wait_established();
    IConn* server_conn1 = server_conn_handler1.wait_established();

    POINTERS_EQUAL(server_conn1, acceptor.wait_added());

    // try to connect to non-listening socket
    TcpClientConfig client_config2 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", client_config1.local_address.port());

    CHECK(add_tcp_client(net_loop, client_config2, client_conn_handler2));

    IConn* client_conn2 = client_conn_handler2.wait_refused();
    expect_read_error(client_conn2, SockErr_Failure);

    terminate_and_wait(server_conn_handler1, server_conn1, test::ExpectNotFailed);
    terminate_and_wait(client_conn_handler1, client_conn1, test::ExpectNotFailed);

    terminate_and_wait(client_conn_handler2, client_conn2, test::ExpectFailed);

    POINTERS_EQUAL(&server_conn_handler1, acceptor.wait_removed());
}

TEST(tcp_ports, acceptor_error) {
    test::MockConnHandler client_conn_handler1;
    test::MockConnHandler client_conn_handler2;
    test::MockConnHandler server_conn_handler2;

    test::MockConnAcceptor acceptor;

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    CHECK(add_tcp_server(net_loop, server_config, acceptor));

    TcpClientConfig client_config1 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    acceptor.drop_next_connection();

    CHECK(add_tcp_client(net_loop, client_config1, client_conn_handler1));

    IConn* client_conn1 = client_conn_handler1.wait_established();

    wait_writable_readable(client_conn_handler1, client_conn1, true, true);
    expect_write_error(client_conn1, SockErr_Failure);
    expect_read_error(client_conn1, SockErr_Failure);

    CHECK(client_conn1->is_failed());

    TcpClientConfig client_config2 = make_client_config(
        "127.0.0.1", 0, "127.0.0.1", server_config.bind_address.port());

    acceptor.push_handler(server_conn_handler2);

    CHECK(add_tcp_client(net_loop, client_config2, client_conn_handler2));

    IConn* server_conn2 = server_conn_handler2.wait_established();
    IConn* client_conn2 = client_conn_handler2.wait_established();

    POINTERS_EQUAL(server_conn2, acceptor.wait_added());

    wait_writable_readable(client_conn_handler2, client_conn2, true, false);

    terminate_and_wait(client_conn_handler1, client_conn1, test::ExpectFailed);

    terminate_and_wait(client_conn_handler2, client_conn2, test::ExpectNotFailed);
    terminate_and_wait(server_conn_handler2, server_conn2, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler2, acceptor.wait_removed());
}

TEST(tcp_ports, terminate_client_connection_normal) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop, client_config, client_conn_handler);
    CHECK(client_handle);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    IConn* client_conn = client_conn_handler.wait_established();
    IConn* server_conn = server_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    wait_writable_readable(server_conn_handler, server_conn, true, false);
    wait_writable_readable(client_conn_handler, client_conn, true, false);

    client_conn->async_terminate(Term_Normal);
    client_conn_handler.wait_terminated(test::ExpectNotFailed);

    wait_writable_readable(server_conn_handler, server_conn, true, true);
    expect_read_error(server_conn, SockErr_StreamEnd);

    CHECK(!server_conn->is_failed());

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());

    remove_port(net_loop, client_handle);
    remove_port(net_loop, server_handle);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(tcp_ports, terminate_client_connection_failure) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop, client_config, client_conn_handler);
    CHECK(client_handle);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    IConn* client_conn = client_conn_handler.wait_established();
    IConn* server_conn = server_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    wait_writable_readable(server_conn_handler, server_conn, true, false);
    wait_writable_readable(client_conn_handler, client_conn, true, false);

    client_conn->async_terminate(Term_Failure);
    client_conn_handler.wait_terminated(test::ExpectFailed);

    wait_writable_readable(server_conn_handler, server_conn, true, true);
    expect_write_error(server_conn, SockErr_Failure);
    expect_read_error(server_conn, SockErr_Failure);

    CHECK(server_conn->is_failed());

    terminate_and_wait(server_conn_handler, server_conn, test::ExpectFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());

    remove_port(net_loop, client_handle);
    remove_port(net_loop, server_handle);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(tcp_ports, terminate_server_connection_normal) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop, client_config, client_conn_handler);
    CHECK(client_handle);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    IConn* client_conn = client_conn_handler.wait_established();
    IConn* server_conn = server_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    wait_writable_readable(server_conn_handler, server_conn, true, false);
    wait_writable_readable(client_conn_handler, client_conn, true, false);

    server_conn->async_terminate(Term_Normal);
    server_conn_handler.wait_terminated(test::ExpectNotFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());

    wait_writable_readable(client_conn_handler, client_conn, true, true);
    expect_read_error(client_conn, SockErr_StreamEnd);

    CHECK(!client_conn->is_failed());

    terminate_and_wait(client_conn_handler, client_conn, test::ExpectNotFailed);

    remove_port(net_loop, client_handle);
    remove_port(net_loop, server_handle);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(tcp_ports, terminate_server_connection_failure) {
    test::MockConnHandler client_conn_handler;
    test::MockConnHandler server_conn_handler;

    test::MockConnAcceptor acceptor;
    acceptor.push_handler(server_conn_handler);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    TcpServerConfig server_config = make_server_config("127.0.0.1", 0);

    NetworkLoop::PortHandle server_handle =
        add_tcp_server(net_loop, server_config, acceptor);
    CHECK(server_handle);

    TcpClientConfig client_config = make_client_config("127.0.0.1", 0, "127.0.0.1",
                                                       server_config.bind_address.port());

    NetworkLoop::PortHandle client_handle =
        add_tcp_client(net_loop, client_config, client_conn_handler);
    CHECK(client_handle);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    IConn* client_conn = client_conn_handler.wait_established();
    IConn* server_conn = server_conn_handler.wait_established();

    POINTERS_EQUAL(server_conn, acceptor.wait_added());

    wait_writable_readable(server_conn_handler, server_conn, true, false);
    wait_writable_readable(client_conn_handler, client_conn, true, false);

    server_conn->async_terminate(Term_Failure);
    server_conn_handler.wait_terminated(test::ExpectFailed);

    POINTERS_EQUAL(&server_conn_handler, acceptor.wait_removed());

    wait_writable_readable(client_conn_handler, client_conn, true, true);
    expect_write_error(client_conn, SockErr_Failure);
    expect_read_error(client_conn, SockErr_Failure);

    CHECK(client_conn->is_failed());

    terminate_and_wait(client_conn_handler, client_conn, test::ExpectFailed);

    remove_port(net_loop, client_handle);
    remove_port(net_loop, server_handle);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

} // namespace netio
} // namespace roc
