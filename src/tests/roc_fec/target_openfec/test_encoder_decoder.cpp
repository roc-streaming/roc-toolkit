/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/random.h"
#include "roc_fec/of_decoder.h"
#include "roc_fec/of_encoder.h"

namespace roc {
namespace fec {

namespace {

const size_t NumSourcePackets = 20;
const size_t NumRepairPackets = 10;

const size_t PayloadSize = 251;

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, PayloadSize, 1);

} // namespace

class Codec {
public:
    Codec(const Config& config)
        : encoder_(config, PayloadSize, allocator)
        , decoder_(config, PayloadSize, buffer_pool, allocator)
        , buffers_(allocator, NumSourcePackets + NumRepairPackets) {
        buffers_.resize(buffers_.max_size());
    }

    void encode() {
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            buffers_[i] = make_buffer_();
            encoder_.set(i, buffers_[i]);
        }
        encoder_.commit();
        encoder_.reset();
    }

    bool decode() {
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            core::Slice<uint8_t> decoded = decoder_.repair(i);
            if (!decoded) {
                return false;
            }

            LONGS_EQUAL(PayloadSize, decoded.size());

            if (memcmp(buffers_[i].data(), decoded.data(), PayloadSize) != 0) {
                return false;
            }
        }
        return true;
    }

    IEncoder& encoder() {
        return encoder_;
    }

    IDecoder& decoder() {
        return decoder_;
    }

    const core::Slice<uint8_t>& get_buffer(const size_t i) {
        return buffers_[i];
    }

private:
    core::Slice<uint8_t> make_buffer_() {
        core::Slice<uint8_t> buf = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        buf.resize(PayloadSize);
        for (size_t j = 0; j < buf.size(); ++j) {
            buf.data()[j] = (uint8_t)core::random(0, 0xff);
        }
        return buf;
    }

    OFEncoder encoder_;
    OFDecoder decoder_;

    core::Array<core::Slice<uint8_t> > buffers_;
};

TEST_GROUP(encoder_decoder) {
    Config config;
    void setup() {
        config.n_source_packets = NumSourcePackets;
        config.n_repair_packets = NumRepairPackets;
    }
};

TEST(encoder_decoder, without_loss) {
    for (int type = ReedSolomon8m; type != CodecTypeMax; ++type) {
        config.codec = (CodecType)type;
        Codec code(config);
        code.encode();
        // Sending all packets in block without loss.
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            code.decoder().set(i, code.get_buffer(i));
        }
        CHECK(code.decode());
    }
}

TEST(encoder_decoder, loss_1) {
    for (int type = ReedSolomon8m; type != CodecTypeMax; ++type) {
        config.codec = (CodecType)type;
        Codec code(config);
        code.encode();
        // Sending all packets in block with one loss.
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            if (i == 5) {
                continue;
            }
            code.decoder().set(i, code.get_buffer(i));
        }
        CHECK(code.decode());
    }
}

TEST(encoder_decoder, load_test) {
    enum { NumIterations = 20, LossPercent = 10, MaxLoss = 3 };
    for (int type = ReedSolomon8m; type != CodecTypeMax; ++type) {
        config.codec = (CodecType)type;
        Codec code(config);

        size_t total_loss = 0;
        size_t max_loss = 0;

        size_t total_fails = 0;

        for (size_t test_num = 0; test_num < NumIterations; ++test_num) {
            code.encode();
            size_t curr_loss = 0;
            for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
                if (core::random(100) < LossPercent && curr_loss <= MaxLoss) {
                    total_loss++;
                    curr_loss++;
                } else {
                    code.decoder().set(i, code.get_buffer(i));
                }
            }
            max_loss = ROC_MAX(max_loss, curr_loss);
            if (!code.decode()) {
                total_fails++;
            }
            code.decoder().reset();
        }

        roc_log(LogInfo, "max losses in block: %u", (uint32_t)max_loss);
        roc_log(LogInfo, "total losses: %u", (uint32_t)total_loss);
        roc_log(LogInfo, "total fails: %u", (uint32_t)total_fails);

        CHECK(total_fails < NumIterations / 2);
    }
}

} // namespace fec
} // namespace roc
