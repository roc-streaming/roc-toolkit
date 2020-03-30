/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/fast_random.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/codec_map.h"

namespace roc {
namespace fec {

namespace {

const size_t MaxPayloadSize = 1024;

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxPayloadSize, true);

} // namespace

class Codec {
public:
    Codec(const CodecConfig& config)
        : encoder_(CodecMap::instance().new_encoder(config, buffer_pool, allocator),
                   allocator)
        , decoder_(CodecMap::instance().new_decoder(config, buffer_pool, allocator),
                   allocator)
        , buffers_(allocator) {
        CHECK(encoder_);
        CHECK(decoder_);
    }

    void encode(size_t n_source, size_t n_repair, size_t p_size) {
        CHECK(buffers_.resize(n_source + n_repair));

        CHECK(encoder_->begin(n_source, n_repair, p_size));

        for (size_t i = 0; i < n_source + n_repair; ++i) {
            buffers_[i] = make_buffer_(p_size);
            encoder_->set(i, buffers_[i]);
        }
        encoder_->fill();
        encoder_->end();
    }

    bool decode(size_t n_source, size_t p_size) {
        for (size_t i = 0; i < n_source; ++i) {
            core::Slice<uint8_t> decoded = decoder_->repair(i);
            if (!decoded) {
                return false;
            }

            UNSIGNED_LONGS_EQUAL(p_size, decoded.size());

            if (memcmp(buffers_[i].data(), decoded.data(), p_size) != 0) {
                return false;
            }
        }
        return true;
    }

    IBlockEncoder& encoder() {
        return *encoder_;
    }

    IBlockDecoder& decoder() {
        return *decoder_;
    }

    const core::Slice<uint8_t>& get_buffer(const size_t i) {
        return buffers_[i];
    }

private:
    core::Slice<uint8_t> make_buffer_(size_t p_size) {
        core::Slice<uint8_t> buf = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        buf.resize(p_size);
        for (size_t j = 0; j < buf.size(); ++j) {
            buf.data()[j] = (uint8_t)core::fast_random(0, 0xff);
        }
        return buf;
    }

    core::ScopedPtr<IBlockEncoder> encoder_;
    core::ScopedPtr<IBlockDecoder> decoder_;

    core::Array<core::Slice<uint8_t> > buffers_;
};

TEST_GROUP(encoder_decoder) {};

TEST(encoder_decoder, without_loss) {
    enum { NumSourcePackets = 20, NumRepairPackets = 10, PayloadSize = 251 };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        Codec code(config);
        code.encode(NumSourcePackets, NumRepairPackets, PayloadSize);

        CHECK(code.decoder().begin(NumSourcePackets, NumRepairPackets, PayloadSize));

        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            code.decoder().set(i, code.get_buffer(i));
        }
        CHECK(code.decode(NumSourcePackets, PayloadSize));

        code.decoder().end();
    }
}

TEST(encoder_decoder, lost_1) {
    enum { NumSourcePackets = 20, NumRepairPackets = 10, PayloadSize = 251 };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        Codec code(config);
        code.encode(NumSourcePackets, NumRepairPackets, PayloadSize);

        CHECK(code.decoder().begin(NumSourcePackets, NumRepairPackets, PayloadSize));

        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            if (i == 5) {
                continue;
            }
            code.decoder().set(i, code.get_buffer(i));
        }
        CHECK(code.decode(NumSourcePackets, PayloadSize));

        code.decoder().end();
    }
}

TEST(encoder_decoder, random_losses) {
    enum {
        NumSourcePackets = 20,
        NumRepairPackets = 10,
        PayloadSize = 251,
        NumIterations = 20,
        LossPercent = 10,
        MaxLoss = 3
    };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        Codec code(config);

        size_t total_loss = 0;
        size_t max_loss = 0;

        size_t total_fails = 0;

        for (size_t test_num = 0; test_num < NumIterations; ++test_num) {
            code.encode(NumSourcePackets, NumRepairPackets, PayloadSize);

            CHECK(code.decoder().begin(NumSourcePackets, NumRepairPackets, PayloadSize));

            size_t curr_loss = 0;
            for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
                if (core::fast_random(0, 100) < LossPercent && curr_loss <= MaxLoss) {
                    total_loss++;
                    curr_loss++;
                } else {
                    code.decoder().set(i, code.get_buffer(i));
                }
            }
            max_loss = std::max(max_loss, curr_loss);
            if (!code.decode(NumSourcePackets, PayloadSize)) {
                total_fails++;
            }

            code.decoder().end();
        }

        roc_log(LogInfo, "max losses in block: %u", (unsigned)max_loss);
        roc_log(LogInfo, "total losses: %u", (unsigned)total_loss);
        roc_log(LogInfo, "total fails: %u", (unsigned)total_fails);

        CHECK(total_fails < NumIterations / 2);
    }
}

TEST(encoder_decoder, full_repair_payload_sizes) {
    enum { NumSourcePackets = 10, NumRepairPackets = 20 };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        for (size_t p_size = 1; p_size < 300; p_size++) {
            roc_log(LogInfo, "payload size %u", (unsigned)p_size);

            Codec code(config);
            code.encode(NumSourcePackets, NumRepairPackets, p_size);

            CHECK(code.decoder().begin(NumSourcePackets, NumRepairPackets, p_size));

            for (size_t i = NumSourcePackets; i < NumSourcePackets + NumRepairPackets;
                 ++i) {
                code.decoder().set(i, code.get_buffer(i));
            }
            CHECK(code.decode(NumSourcePackets, p_size));

            code.decoder().end();
        }
    }
}

TEST(encoder_decoder, max_source_block) {
    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); ++n_scheme) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        Codec code(config);

        CHECK(code.encoder().max_block_length() > 0);
        CHECK(code.decoder().max_block_length() > 0);
    }
}

} // namespace fec
} // namespace roc
