/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_arena.h"

#include "roc_core/array.h"
#include "roc_core/fast_random.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/codec_map.h"

namespace roc {
namespace fec {

namespace {

const size_t MaxPayloadSize = 1024;

} // namespace

class Codec {
public:
    Codec(const CodecConfig& config)
        : arena_()
        , packet_factory_(arena_, MaxPayloadSize)
        , encoder_(
              CodecMap::instance().new_block_encoder(config, packet_factory_, arena_))
        , decoder_(
              CodecMap::instance().new_block_decoder(config, packet_factory_, arena_))
        , buffers_(arena_) {
        set_fail(false);

        CHECK(encoder_);
        CHECK(decoder_);

        LONGS_EQUAL(status::StatusOK, encoder_->init_status());
        LONGS_EQUAL(status::StatusOK, decoder_->init_status());
    }

    void encode(size_t n_source, size_t n_repair, size_t p_size) {
        CHECK(buffers_.resize(n_source + n_repair));

        LONGS_EQUAL(status::StatusOK, encoder_->begin_block(n_source, n_repair, p_size));

        for (size_t i = 0; i < n_source + n_repair; ++i) {
            buffers_[i] = make_buffer_(p_size);
            encoder_->set_buffer(i, buffers_[i]);
        }
        encoder_->fill_buffers();
        encoder_->end_block();
    }

    bool decode(size_t n_source, size_t p_size) {
        for (size_t i = 0; i < n_source; ++i) {
            core::Slice<uint8_t> decoded = decoder_->repair_buffer(i);
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

    void set_fail(bool fail) {
        arena_.set_fail(fail);
    }

private:
    core::Slice<uint8_t> make_buffer_(size_t p_size) {
        core::Slice<uint8_t> buf = packet_factory_.new_packet_buffer();
        buf.reslice(0, p_size);
        for (size_t j = 0; j < buf.size(); ++j) {
            buf.data()[j] = (uint8_t)core::fast_random_range(0, 0xff);
        }
        return buf;
    }

    test::MockArena arena_;
    packet::PacketFactory packet_factory_;
    core::ScopedPtr<IBlockEncoder> encoder_;
    core::ScopedPtr<IBlockDecoder> decoder_;
    core::Array<core::Slice<uint8_t> > buffers_;
};

TEST_GROUP(block_encoder_decoder) {};

TEST(block_encoder_decoder, without_loss) {
    enum { NumSourcePackets = 20, NumRepairPackets = 10, PayloadSize = 251 };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        Codec code(config);
        code.encode(NumSourcePackets, NumRepairPackets, PayloadSize);

        LONGS_EQUAL(
            status::StatusOK,
            code.decoder().begin_block(NumSourcePackets, NumRepairPackets, PayloadSize));

        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            code.decoder().set_buffer(i, code.get_buffer(i));
        }
        CHECK(code.decode(NumSourcePackets, PayloadSize));

        code.decoder().end_block();
    }
}

TEST(block_encoder_decoder, lost_1) {
    enum { NumSourcePackets = 20, NumRepairPackets = 10, PayloadSize = 251 };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        Codec code(config);
        code.encode(NumSourcePackets, NumRepairPackets, PayloadSize);

        LONGS_EQUAL(
            status::StatusOK,
            code.decoder().begin_block(NumSourcePackets, NumRepairPackets, PayloadSize));

        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            if (i == 5) {
                continue;
            }
            code.decoder().set_buffer(i, code.get_buffer(i));
        }
        CHECK(code.decode(NumSourcePackets, PayloadSize));

        code.decoder().end_block();
    }
}

TEST(block_encoder_decoder, random_losses) {
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

            LONGS_EQUAL(status::StatusOK,
                        code.decoder().begin_block(NumSourcePackets, NumRepairPackets,
                                                   PayloadSize));

            size_t curr_loss = 0;
            for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
                if (core::fast_random_range(0, 100) < LossPercent
                    && curr_loss <= MaxLoss) {
                    total_loss++;
                    curr_loss++;
                } else {
                    code.decoder().set_buffer(i, code.get_buffer(i));
                }
            }
            max_loss = std::max(max_loss, curr_loss);
            if (!code.decode(NumSourcePackets, PayloadSize)) {
                total_fails++;
            }

            code.decoder().end_block();
        }

        roc_log(LogInfo, "max losses in block: %u", (unsigned)max_loss);
        roc_log(LogInfo, "total losses: %u", (unsigned)total_loss);
        roc_log(LogInfo, "total fails: %u", (unsigned)total_fails);

        CHECK(total_fails < NumIterations / 2);
    }
}

TEST(block_encoder_decoder, full_repair_payload_sizes) {
    enum { NumSourcePackets = 10, NumRepairPackets = 20 };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        for (size_t p_size = 1; p_size < 300; p_size++) {
            roc_log(LogInfo, "payload size %u", (unsigned)p_size);

            Codec code(config);
            code.encode(NumSourcePackets, NumRepairPackets, p_size);

            LONGS_EQUAL(
                status::StatusOK,
                code.decoder().begin_block(NumSourcePackets, NumRepairPackets, p_size));

            for (size_t i = NumSourcePackets; i < NumSourcePackets + NumRepairPackets;
                 ++i) {
                code.decoder().set_buffer(i, code.get_buffer(i));
            }
            CHECK(code.decode(NumSourcePackets, p_size));

            code.decoder().end_block();
        }
    }
}

TEST(block_encoder_decoder, no_memory) {
    enum { NumSourcePackets = 20, NumRepairPackets = 10, PayloadSize = 251 };

    for (size_t n_scheme = 0; n_scheme < CodecMap::instance().num_schemes(); n_scheme++) {
        CodecConfig config;
        config.scheme = CodecMap::instance().nth_scheme(n_scheme);

        { // test encoder
            Codec code(config);
            code.set_fail(true);
            LONGS_EQUAL(status::StatusNoMem,
                        code.encoder().begin_block(NumSourcePackets, NumRepairPackets,
                                                   PayloadSize));
        }

        { // test decoder
            Codec code(config);
            code.encode(NumSourcePackets, NumRepairPackets, PayloadSize);
            code.set_fail(true);
            LONGS_EQUAL(status::StatusNoMem,
                        code.decoder().begin_block(NumSourcePackets, NumRepairPackets,
                                                   PayloadSize));
        }
    }
}

TEST(block_encoder_decoder, max_source_block) {
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
