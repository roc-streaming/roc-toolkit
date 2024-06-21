/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_PACKET_WRITER_H_
#define ROC_PIPELINE_TEST_HELPERS_PACKET_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_audio/iframe_encoder.h"
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/block_writer.h"
#include "roc_fec/codec_map.h"
#include "roc_fec/composer.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_packet/fec.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/encoding_map.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {
namespace test {

// Generates source and repair packets and pass them to destination writers
class PacketWriter : public core::NonCopyable<> {
public:
    // Initialize without FEC (produce only source packets)
    PacketWriter(core::IArena& arena,
                 packet::IWriter& dst_writer,
                 rtp::EncodingMap& encoding_map,
                 packet::PacketFactory& packet_factory,
                 packet::stream_source_t src_id,
                 const address::SocketAddr& src_addr,
                 const address::SocketAddr& dst_addr,
                 rtp::PayloadType pt)
        : source_writer_(&dst_writer)
        , repair_writer_(NULL)
        , packet_factory_(packet_factory)
        , src_addr_(src_addr)
        , source_dst_addr_(dst_addr)
        , repair_dst_addr_()
        , source_(src_id)
        , seqnum_(0)
        , timestamp_(0)
        , pt_(pt)
        , offset_(0)
        , corrupt_(false) {
        construct_(arena, packet_factory, encoding_map, pt, packet::FEC_None,
                   fec::BlockWriterConfig());
    }

    // Initialize with FEC (produce source + repair packets)
    PacketWriter(core::IArena& arena,
                 packet::IWriter& source_dst_writer,
                 packet::IWriter& repair_dst_writer,
                 rtp::EncodingMap& encoding_map,
                 packet::PacketFactory& packet_factory,
                 packet::stream_source_t src_id,
                 const address::SocketAddr& src_addr,
                 const address::SocketAddr& source_dst_addr,
                 const address::SocketAddr& repair_dst_addr,
                 rtp::PayloadType pt,
                 packet::FecScheme fec_scheme,
                 fec::BlockWriterConfig fec_config)
        : source_writer_(&source_dst_writer)
        , repair_writer_(&repair_dst_writer)
        , packet_factory_(packet_factory)
        , src_addr_(src_addr)
        , source_dst_addr_(source_dst_addr)
        , repair_dst_addr_(repair_dst_addr)
        , source_(src_id)
        , seqnum_(0)
        , timestamp_(0)
        , pt_(pt)
        , offset_(0)
        , corrupt_(false) {
        construct_(arena, packet_factory, encoding_map, pt, fec_scheme, fec_config);
    }

    void write_packets(size_t num_packets,
                       size_t samples_per_packet,
                       const audio::SampleSpec& sample_spec) {
        CHECK(num_packets > 0);

        for (size_t np = 0; np < num_packets; np++) {
            packet::PacketPtr pp = create_packet_(samples_per_packet, sample_spec);
            CHECK(pp);
            deliver_packet_(pp);
        }
    }

    void shift_to(size_t num_packets, size_t samples_per_packet) {
        seqnum_ = packet::seqnum_t(num_packets);
        timestamp_ = packet::stream_timestamp_t(num_packets * samples_per_packet);
        offset_ = uint8_t(num_packets * samples_per_packet);
    }

    uint8_t offset() const {
        return offset_;
    }

    void set_offset(size_t offset) {
        offset_ = uint8_t(offset);
    }

    packet::stream_source_t source() const {
        return source_;
    }

    void set_source(packet::stream_source_t source) {
        source_ = source;
    }

    packet::seqnum_t seqnum() const {
        return seqnum_;
    }

    void set_seqnum(packet::seqnum_t seqnum) {
        seqnum_ = seqnum;
    }

    packet::stream_timestamp_t timestamp() const {
        return timestamp_;
    }

    void set_timestamp(packet::stream_timestamp_t timestamp) {
        timestamp_ = timestamp;
    }

    void set_corrupt(bool corrupt) {
        corrupt_ = corrupt;
    }

private:
    enum { MaxSamples = 4096 };

    void construct_(core::IArena& arena,
                    packet::PacketFactory& packet_factory,
                    rtp::EncodingMap& encoding_map,
                    rtp::PayloadType pt,
                    packet::FecScheme fec_scheme,
                    fec::BlockWriterConfig fec_config) {
        // payload encoder
        const rtp::Encoding* enc = encoding_map.find_by_pt(pt);
        CHECK(enc);
        payload_encoder_.reset(enc->new_encoder(arena, enc->sample_spec), arena);
        CHECK(payload_encoder_);

        if (fec_scheme == packet::FEC_None) {
            // rtp composer
            source_composer_.reset(new (arena) rtp::Composer(NULL), arena);
        } else {
            if (fec_scheme == packet::FEC_ReedSolomon_M8) {
                // rs8m composers
                payload_composer_.reset(new (arena) rtp::Composer(NULL), arena);

                source_composer_.reset(
                    new (arena)
                        fec::Composer<fec::RS8M_PayloadID, fec::Source, fec::Footer>(
                            payload_composer_.get()),
                    arena);

                repair_composer_.reset(
                    new (arena)
                        fec::Composer<fec::RS8M_PayloadID, fec::Repair, fec::Header>(
                            NULL),
                    arena);
            } else if (fec_scheme == packet::FEC_LDPC_Staircase) {
                // ldpc composers
                payload_composer_.reset(new (arena) rtp::Composer(NULL), arena);

                source_composer_.reset(
                    new (arena) fec::Composer<fec::LDPC_Source_PayloadID, fec::Source,
                                              fec::Footer>(payload_composer_.get()),
                    arena);

                repair_composer_.reset(new (arena)
                                           fec::Composer<fec::LDPC_Repair_PayloadID,
                                                         fec::Repair, fec::Header>(NULL),
                                       arena);
            }

            // fec encoder
            fec::CodecConfig codec_config;
            codec_config.scheme = fec_scheme;
            fec_encoder_.reset(fec::CodecMap::instance().new_encoder(
                                   codec_config, packet_factory, arena),
                               arena);
            CHECK(fec_encoder_);
            LONGS_EQUAL(status::StatusOK, fec_encoder_->init_status());

            // fec writer
            fec_writer_.reset(
                new (arena) fec::BlockWriter(fec_config, fec_scheme, *fec_encoder_,
                                             fec_queue_, *source_composer_,
                                             *repair_composer_, packet_factory, arena),
                arena);
            CHECK(fec_writer_);
            LONGS_EQUAL(status::StatusOK, fec_writer_->init_status());
        }
    }

    void deliver_packet_(const packet::PacketPtr& pp) {
        if (fec_writer_) {
            // fec_writer will produce source and repair packets and store in fec_queue
            // note that we're calling copy_packet_() only after fec_writer, because
            // fec writer normally lives in the middle of the pipeline and expects
            // packets to have all necessary meta-information
            LONGS_EQUAL(status::StatusOK, fec_writer_->write(pp));

            // compose and "deliver" source and repair packets produced by fec_writer
            packet::PacketPtr fp;
            while (fec_queue_.read(fp) == status::StatusOK) {
                if (fp->has_flags(packet::Packet::FlagAudio)) {
                    CHECK(source_composer_->compose(*fp));
                    LONGS_EQUAL(status::StatusOK,
                                source_writer_->write(copy_packet_(fp)));
                } else {
                    CHECK(repair_composer_->compose(*fp));
                    LONGS_EQUAL(status::StatusOK,
                                repair_writer_->write(copy_packet_(fp)));
                }
            }
        } else {
            // compose and "deliver" packet
            CHECK(source_composer_->compose(*pp));
            LONGS_EQUAL(status::StatusOK, source_writer_->write(copy_packet_(pp)));
        }
    }

    // creates a new packet with the same buffer, without copying any meta-information
    // like flags, parsed fields, etc; this way we simulate that packet was "delivered"
    // over network - packets enters receiver's pipeline without any meta-information,
    // and receiver fills that meta-information using packet parsers
    packet::PacketPtr copy_packet_(const packet::PacketPtr& pa) {
        packet::PacketPtr pb = packet_factory_.new_packet();
        CHECK(pb);

        pb->add_flags(packet::Packet::FlagUDP);
        pb->udp()->src_addr = src_addr_;
        pb->udp()->dst_addr = source_dst_addr_;

        pb->set_buffer(pa->buffer());

        if (corrupt_) {
            pb->buffer().data()[0] = 0;
        }

        return pb;
    }

    // creates next source packet
    packet::PacketPtr create_packet_(size_t samples_per_packet,
                                     const audio::SampleSpec& sample_spec) {
        CHECK(samples_per_packet * sample_spec.num_channels() < MaxSamples);

        packet::PacketPtr pp = packet_factory_.new_packet();
        CHECK(pp);

        pp->add_flags(packet::Packet::FlagAudio);

        core::Slice<uint8_t> bp = packet_factory_.new_packet_buffer();
        CHECK(bp);

        CHECK(source_composer_->prepare(
            *pp, bp, payload_encoder_->encoded_byte_count(samples_per_packet)));

        pp->set_buffer(bp);

        pp->rtp()->source_id = source_;
        pp->rtp()->seqnum = seqnum_;
        pp->rtp()->stream_timestamp = timestamp_;
        pp->rtp()->payload_type = pt_;

        seqnum_++;
        timestamp_ += samples_per_packet;

        audio::sample_t samples[MaxSamples];
        for (size_t ns = 0; ns < samples_per_packet; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                samples[ns * sample_spec.num_channels() + nc] = nth_sample(offset_);
            }
            offset_++;
        }

        payload_encoder_->begin(pp->rtp()->payload.data(), pp->rtp()->payload.size());

        UNSIGNED_LONGS_EQUAL(samples_per_packet,
                             payload_encoder_->write(samples, samples_per_packet));

        payload_encoder_->end();

        return pp;
    }

    packet::IWriter* source_writer_;
    packet::IWriter* repair_writer_;

    core::ScopedPtr<packet::IComposer> payload_composer_;
    core::ScopedPtr<packet::IComposer> source_composer_;
    core::ScopedPtr<packet::IComposer> repair_composer_;

    core::ScopedPtr<fec::IBlockEncoder> fec_encoder_;
    core::ScopedPtr<fec::BlockWriter> fec_writer_;
    packet::Queue fec_queue_;

    core::ScopedPtr<audio::IFrameEncoder> payload_encoder_;

    packet::PacketFactory& packet_factory_;

    address::SocketAddr src_addr_;
    address::SocketAddr source_dst_addr_;
    address::SocketAddr repair_dst_addr_;

    packet::stream_source_t source_;
    packet::seqnum_t seqnum_;
    packet::stream_timestamp_t timestamp_;

    rtp::PayloadType pt_;

    uint8_t offset_;

    bool corrupt_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_PACKET_WRITER_H_
