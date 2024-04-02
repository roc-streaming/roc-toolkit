/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/spsc_byte_buffer.h"

namespace roc {
namespace core {

namespace {

HeapArena arena;

void fill_bytes(uint8_t* bytes, size_t num_bytes, uint8_t value) {
    for (size_t i = 0; i < num_bytes; i++) {
        bytes[i] = value;
    }
}

void expect_bytes(const uint8_t* bytes, size_t num_bytes, uint8_t value) {
    for (size_t i = 0; i < num_bytes; i++) {
        LONGS_EQUAL(value, bytes[i]);
    }
}

} // namespace

TEST_GROUP(spsc_byte_buffer) {};

TEST(spsc_byte_buffer, write_before_read) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    for (int i = 0; i < IterCount; i++) {
        // write
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        fill_bytes(wr_bytes, ChunkSize, i + 1);
        sb.end_write();

        // read
        const uint8_t* rd_bytes = sb.begin_read();
        CHECK(rd_bytes);
        expect_bytes(rd_bytes, ChunkSize, i + 1);
        sb.end_read();
    }
}

TEST(spsc_byte_buffer, read_before_write) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    for (int i = 0; i < IterCount; i++) {
        // read
        const uint8_t* rd_bytes = sb.begin_read();
        if (i == 0) {
            CHECK(!rd_bytes);
        } else {
            CHECK(rd_bytes);
            expect_bytes(rd_bytes, ChunkSize, i);
            sb.end_read();
        }

        // write
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        fill_bytes(wr_bytes, ChunkSize, i + 1);
        sb.end_write();
    }
}

TEST(spsc_byte_buffer, read_inside_write) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    for (int i = 0; i < IterCount; i++) {
        // begin write
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        fill_bytes(wr_bytes, ChunkSize, i + 1);

        // read
        const uint8_t* rd_bytes = sb.begin_read();
        if (i == 0) {
            CHECK(!rd_bytes);
        } else {
            CHECK(rd_bytes);
            expect_bytes(rd_bytes, ChunkSize, i);
            sb.end_read();
        }

        // end write
        sb.end_write();
    }
}

TEST(spsc_byte_buffer, write_inside_read) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    for (int i = 0; i < IterCount; i++) {
        // begin read
        const uint8_t* rd_bytes = sb.begin_read();

        // write
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        fill_bytes(wr_bytes, ChunkSize, i + 1);
        sb.end_write();

        // end read
        if (i == 0) {
            CHECK(!rd_bytes);
        } else {
            CHECK(rd_bytes);
            expect_bytes(rd_bytes, ChunkSize, i);
            sb.end_read();
        }
    }
}

TEST(spsc_byte_buffer, interleaved_write_read) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    for (int i = 0; i < IterCount; i++) {
        // begin write
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        fill_bytes(wr_bytes, ChunkSize, i + 1);

        // begin read
        const uint8_t* rd_bytes = sb.begin_read();

        // end write
        sb.end_write();

        // end read
        if (i == 0) {
            CHECK(!rd_bytes);
        } else {
            CHECK(rd_bytes);
            expect_bytes(rd_bytes, ChunkSize, i);
            sb.end_read();
        }
    }
}

TEST(spsc_byte_buffer, interleaved_read_write) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    for (int i = 0; i < IterCount; i++) {
        // begin read
        const uint8_t* rd_bytes = sb.begin_read();

        // begin write
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        fill_bytes(wr_bytes, ChunkSize, i + 1);

        // end read
        if (i == 0) {
            CHECK(!rd_bytes);
        } else {
            CHECK(rd_bytes);
            expect_bytes(rd_bytes, ChunkSize, i);
            sb.end_read();
        }

        // end write
        sb.end_write();
    }
}

TEST(spsc_byte_buffer, overrun) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    int wr_pos = 0, rd_pos = 0;

    for (int i = 0; i < ChunkCount - 1; i++) {
        // write
        wr_pos++;
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        fill_bytes(wr_bytes, ChunkSize, wr_pos);
        sb.end_write();
    }

    for (int i = 0; i < IterCount; i++) {
        { // write
            wr_pos++;
            uint8_t* wr_bytes = sb.begin_write();
            CHECK(wr_bytes);
            fill_bytes(wr_bytes, ChunkSize, wr_pos);
            sb.end_write();
        }

        { // overrun
            uint8_t* wr_bytes = sb.begin_write();
            CHECK(!wr_bytes);
        }

        { // read
            rd_pos++;
            const uint8_t* rd_bytes = sb.begin_read();
            CHECK(rd_bytes);
            expect_bytes(rd_bytes, ChunkSize, rd_pos);
            sb.end_read();
        }
    }

    for (int i = 0; i < ChunkCount - 1; i++) {
        // read
        rd_pos++;
        const uint8_t* rd_bytes = sb.begin_read();
        CHECK(rd_bytes);
        expect_bytes(rd_bytes, ChunkSize, rd_pos);
        sb.end_read();
    }

    { // eof
        const uint8_t* rd_bytes = sb.begin_read();
        CHECK(!rd_bytes);
    }
}

TEST(spsc_byte_buffer, underrun) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    int wr_pos = 0, rd_pos = 0;

    for (int i = 0; i < IterCount; i++) {
        { // write
            wr_pos++;
            uint8_t* wr_bytes = sb.begin_write();
            CHECK(wr_bytes);
            fill_bytes(wr_bytes, ChunkSize, wr_pos);
            sb.end_write();
        }

        { // read
            rd_pos++;
            const uint8_t* rd_bytes = sb.begin_read();
            CHECK(rd_bytes);
            expect_bytes(rd_bytes, ChunkSize, rd_pos);
            sb.end_read();
        }

        { // underrun
            const uint8_t* rd_bytes = sb.begin_read();
            CHECK(!rd_bytes);
        }
    }

    { // eof
        const uint8_t* rd_bytes = sb.begin_read();
        CHECK(!rd_bytes);
    }
}

TEST(spsc_byte_buffer, is_empty) {
    enum { ChunkSize = 33, ChunkCount = 11, IterCount = 100 };

    SpscByteBuffer sb(arena, ChunkSize, ChunkCount);
    CHECK(sb.is_valid());

    for (int i = 0; i < IterCount; i++) {
        // check
        CHECK(sb.is_empty());

        // write
        uint8_t* wr_bytes = sb.begin_write();
        CHECK(wr_bytes);
        sb.end_write();

        // check
        CHECK(!sb.is_empty());

        // read
        const uint8_t* rd_bytes = sb.begin_read();
        CHECK(rd_bytes);
        sb.end_read();

        // check
        CHECK(sb.is_empty());
    }
}

} // namespace core
} // namespace roc
