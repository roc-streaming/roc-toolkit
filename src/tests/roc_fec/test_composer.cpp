/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_fec/composer.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace fec {

TEST_GROUP(composer) {};

TEST(composer, align_footer) {
    enum { BufferSize = 100, Alignment = 8 };

    core::HeapAllocator allocator;
    core::BufferPool<uint8_t> buffer_pool(allocator, BufferSize, true);

    core::Buffer<uint8_t>* buffer = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
    CHECK(buffer);
    CHECK((unsigned long)buffer->data() % Alignment == 0);

    core::Slice<uint8_t> slice(*buffer, 0, 0);
    CHECK(slice);

    UNSIGNED_LONGS_EQUAL(0, slice.size());
    UNSIGNED_LONGS_EQUAL(BufferSize, slice.capacity());
    UNSIGNED_LONGS_EQUAL((unsigned long)buffer->data(), slice.data());

    Composer<RS8M_PayloadID, Source, Footer> composer(NULL);
    CHECK(composer.align(slice, 0, Alignment));

    UNSIGNED_LONGS_EQUAL(0, slice.size());
    UNSIGNED_LONGS_EQUAL(BufferSize, slice.capacity());
    UNSIGNED_LONGS_EQUAL((unsigned long)buffer->data(), slice.data());
}

TEST(composer, align_header) {
    enum { BufferSize = 100, Alignment = 8 };

    core::HeapAllocator allocator;
    core::BufferPool<uint8_t> buffer_pool(allocator, BufferSize, true);

    core::Buffer<uint8_t>* buffer = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
    CHECK(buffer);
    CHECK((unsigned long)buffer->data() % Alignment == 0);

    core::Slice<uint8_t> slice(*buffer, 0, 0);
    CHECK(slice);

    UNSIGNED_LONGS_EQUAL(0, slice.size());
    UNSIGNED_LONGS_EQUAL(BufferSize, slice.capacity());
    UNSIGNED_LONGS_EQUAL((unsigned long)buffer->data(), slice.data());
    CHECK(((unsigned long)slice.data() + sizeof(RS8M_PayloadID)) % Alignment != 0);

    Composer<RS8M_PayloadID, Source, Header> composer(NULL);
    CHECK(composer.align(slice, 0, Alignment));

    UNSIGNED_LONGS_EQUAL(0, slice.size());
    UNSIGNED_LONGS_EQUAL(BufferSize - (Alignment - sizeof(RS8M_PayloadID)),
                         slice.capacity());
    UNSIGNED_LONGS_EQUAL((unsigned long)buffer->data()
                             + (Alignment - sizeof(RS8M_PayloadID)),
                         slice.data());
    CHECK(((unsigned long)slice.data() + sizeof(RS8M_PayloadID)) % Alignment == 0);
}

TEST(composer, align_outer_header) {
    enum { BufferSize = 100, Alignment = 8, OuterHeader = 5 };

    core::HeapAllocator allocator;
    core::BufferPool<uint8_t> buffer_pool(allocator, BufferSize, true);

    core::Buffer<uint8_t>* buffer = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
    CHECK(buffer);
    CHECK((unsigned long)buffer->data() % Alignment == 0);

    core::Slice<uint8_t> slice(*buffer, 0, 0);
    CHECK(slice);

    UNSIGNED_LONGS_EQUAL(0, slice.size());
    UNSIGNED_LONGS_EQUAL(BufferSize, slice.capacity());
    UNSIGNED_LONGS_EQUAL((unsigned long)buffer->data(), slice.data());
    CHECK(((unsigned long)slice.data() + sizeof(RS8M_PayloadID) + OuterHeader) % Alignment
          != 0);

    Composer<RS8M_PayloadID, Source, Header> composer(NULL);
    CHECK(composer.align(slice, OuterHeader, Alignment));

    UNSIGNED_LONGS_EQUAL(0, slice.size());
    UNSIGNED_LONGS_EQUAL(BufferSize
                             - (Alignment * 2 - (sizeof(RS8M_PayloadID) + OuterHeader)),
                         slice.capacity());
    UNSIGNED_LONGS_EQUAL((unsigned long)buffer->data()
                             + (Alignment * 2 - (sizeof(RS8M_PayloadID) + OuterHeader)),
                         slice.data());
    CHECK(((unsigned long)slice.data() + sizeof(RS8M_PayloadID) + OuterHeader) % Alignment
          == 0);
}

TEST(composer, packet_size) {
    enum { BufferSize = 100, Alignment = 8, PayloadSize = 10 };

    core::HeapAllocator allocator;
    core::BufferPool<uint8_t> buffer_pool(allocator, BufferSize, true);
    packet::PacketPool packet_pool(allocator, true);

    core::Slice<uint8_t> buffer = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
    CHECK(buffer);

    packet::PacketPtr packet = new (packet_pool) packet::Packet(packet_pool);
    CHECK(packet);

    Composer<RS8M_PayloadID, Source, Header> composer(NULL);

    CHECK(composer.align(buffer, 0, Alignment));
    CHECK(composer.prepare(*packet, buffer, PayloadSize));

    packet->set_data(buffer);

    CHECK(composer.compose(*packet));

    UNSIGNED_LONGS_EQUAL(sizeof(RS8M_PayloadID) + PayloadSize, packet->data().size());
}

} // namespace fec
} // namespace roc
