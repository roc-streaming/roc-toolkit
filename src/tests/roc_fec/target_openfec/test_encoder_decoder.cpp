/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_fec_schemes.h"

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
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
core::BufferPool<uint8_t> buffer_pool(allocator, PayloadSize, true);

} // namespace

class Codec {
public:
    Codec(const CodecConfig& config)
        : encoder_(config, allocator)
        , decoder_(config, buffer_pool, allocator)
        , buffers_(allocator) {
        CHECK(buffers_.resize(NumSourcePackets + NumRepairPackets));
    }

    void encode() {
        CHECK(encoder_.begin(NumSourcePackets, NumRepairPackets, PayloadSize));

        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            buffers_[i] = make_buffer_();
            encoder_.set(i, buffers_[i]);
        }
        encoder_.fill();
        encoder_.end();
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

TEST_GROUP(encoder_decoder){};

TEST(encoder_decoder, without_loss) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        CodecConfig config;
        config.scheme = Test_fec_schemes[n_scheme];

        Codec code(config);
        code.encode();

        // Sending all packets in block without loss.
        CHECK(code.decoder().begin(NumSourcePackets, NumRepairPackets, PayloadSize));

        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            code.decoder().set(i, code.get_buffer(i));
        }
        CHECK(code.decode());

        code.decoder().end();
    }
}

TEST(encoder_decoder, loss_1) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        CodecConfig config;
        config.scheme = Test_fec_schemes[n_scheme];

        Codec code(config);
        code.encode();

        // Sending all packets in block with one loss.
        CHECK(code.decoder().begin(NumSourcePackets, NumRepairPackets, PayloadSize));

        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            if (i == 5) {
                continue;
            }
            code.decoder().set(i, code.get_buffer(i));
        }
        CHECK(code.decode());

        code.decoder().end();
    }
}

TEST(encoder_decoder, load_test) {
    enum { NumIterations = 20, LossPercent = 10, MaxLoss = 3 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        CodecConfig config;
        config.scheme = Test_fec_schemes[n_scheme];

        Codec code(config);

        size_t total_loss = 0;
        size_t max_loss = 0;

        size_t total_fails = 0;

        for (size_t test_num = 0; test_num < NumIterations; ++test_num) {
            code.encode();

            CHECK(code.decoder().begin(NumSourcePackets, NumRepairPackets, PayloadSize));

            size_t curr_loss = 0;
            for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
                if (core::random(100) < LossPercent && curr_loss <= MaxLoss) {
                    total_loss++;
                    curr_loss++;
                } else {
                    code.decoder().set(i, code.get_buffer(i));
                }
            }
            max_loss = std::max(max_loss, curr_loss);
            if (!code.decode()) {
                total_fails++;
            }

            code.decoder().end();
        }

        roc_log(LogInfo, "max losses in block: %u", (uint32_t)max_loss);
        roc_log(LogInfo, "total losses: %u", (uint32_t)total_loss);
        roc_log(LogInfo, "total fails: %u", (uint32_t)total_fails);

        CHECK(total_fails < NumIterations / 2);
    }
}

TEST(encoder_decoder, max_source_block) {
    size_t test_cases[] = { OF_REED_SOLOMON_MAX_NB_ENCODING_SYMBOLS_DEFAULT,
                            OF_LDPC_STAIRCASE_MAX_NB_ENCODING_SYMBOLS_DEFAULT };

    CHECK(ROC_ARRAY_SIZE(test_cases) == Test_n_fec_schemes);

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; ++n_scheme) {
        CodecConfig config;
        config.scheme = Test_fec_schemes[n_scheme];

        Codec code(config);

        CHECK(code.encoder().max_block_length() == test_cases[n_scheme]);
        CHECK(code.decoder().max_block_length() == test_cases[n_scheme]);
    }
}

} // namespace fec
} // namespace roc
