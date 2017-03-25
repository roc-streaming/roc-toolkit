/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_config/config.h"

#include "roc_core/byte_buffer.h"
#include "roc_core/stddefs.h"
#include "roc_core/log.h"
#include "roc_core/random.h"
#include "roc_core/math.h"
#include "roc_core/array.h"

#include "roc_fec/of_block_encoder.h"
#include "roc_fec/of_block_decoder.h"

namespace roc {
namespace test {

using namespace fec;

namespace {

const size_t N_DATA_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_DATA_PACKETS;
const size_t N_FEC_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_REDUNDANT_PACKETS;

const size_t SYMB_SZ = ROC_CONFIG_DEFAULT_PACKET_SIZE;

} // namespace

TEST_GROUP(block_codecs) {
    OF_BlockEncoder encoder;
    OF_BlockDecoder decoder;

    core::Array<core::IByteBufferConstSlice, N_DATA_PACKETS + N_FEC_PACKETS> buffers;

    void setup() {
        buffers.resize(N_DATA_PACKETS + N_FEC_PACKETS);
    }

    core::IByteBufferConstSlice male_buffer() {
        core::IByteBufferPtr buffer =
            core::ByteBufferTraits::default_composer<SYMB_SZ>().compose();

        buffer->set_size(SYMB_SZ);

        for (size_t j = 0; j < buffer->size(); ++j) {
            buffer->data()[j] = (uint8_t)core::random(0, 0xff);
        }

        return *buffer;
    }

    void encode() {
        for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
            buffers[i] = male_buffer();
            encoder.write(i, buffers[i]);
        }
        encoder.commit();
        for (size_t i = 0; i < N_FEC_PACKETS; ++i) {
            buffers[N_DATA_PACKETS + i] = encoder.read(i);
        }
        encoder.reset();
    }

    bool decode() {
        for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
            core::IByteBufferConstSlice decoded = decoder.repair(i);
            if (!decoded) {
                return false;
            }

            LONGS_EQUAL(SYMB_SZ, decoded.size());

            if (memcmp(buffers[i].data(), decoded.data(), SYMB_SZ) != 0) {
                return false;
            }
        }
        return true;
    }
};

TEST(block_codecs, without_loss) {
    encode();
    // Sending all packets in block without loss.
    for (size_t i = 0; i < N_DATA_PACKETS + N_FEC_PACKETS; ++i) {
        decoder.write(i, buffers[i]);
    }
    CHECK(decode());
}

TEST(block_codecs, loss_1) {
    encode();
    // Sending all packets in block with one loss.
    for (size_t i = 0; i < N_DATA_PACKETS + N_FEC_PACKETS; ++i) {
        if (i == 5) {
            continue;
        }
        decoder.write(i, buffers[i]);
    }
    CHECK(decode());
}

TEST(block_codecs, load_test) {
    enum { NumIterations = 20, LossPercent = 10, MaxLoss = 3 };

    size_t total_loss = 0;
    size_t max_loss = 0;

    size_t total_fails = 0;

    for (size_t test_num = 0; test_num < NumIterations; ++test_num) {
        encode();
        size_t curr_loss = 0;
        for (size_t i = 0; i < N_DATA_PACKETS + N_FEC_PACKETS; ++i) {
            if (core::random(100) < LossPercent && curr_loss <= MaxLoss) {
                total_loss++;
                curr_loss++;
            } else {
                decoder.write(i, buffers[i]);
            }
        }
        max_loss = ROC_MAX(max_loss, curr_loss);
        if (!decode()) {
            total_fails++;
        }
        decoder.reset();
    }

    roc_log(LogInfo, "max losses in block: %u", (uint32_t)max_loss);
    roc_log(LogInfo, "total losses: %u", (uint32_t)total_loss);
    roc_log(LogInfo, "total fails: %u", (uint32_t)total_fails);

    CHECK(total_fails < NumIterations / 2);
}

} // namespace test
} // namespace roc
