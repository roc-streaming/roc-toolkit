/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include <stdio.h>
#include <unistd.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/random.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/queue.h"

#include "roc/address.h"
#include "roc/context.h"
#include "roc/log.h"
#include "roc/receiver.h"
#include "roc/sender.h"

namespace roc {

namespace {

enum {
    MaxBufSize = 500,
    PoolChunkSize = 10000,

    NumChans = 2,

    SourcePackets = 10,
    RepairPackets = 5,

    PacketSamples = 100,
    FrameSamples = PacketSamples * 2,
    TotalSamples = PacketSamples * SourcePackets * 3,

    Latency = TotalSamples / NumChans,
    Timeout = TotalSamples * 10
};

core::HeapAllocator allocator;
packet::PacketPool packet_pool(allocator, PoolChunkSize, true);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, PoolChunkSize, true);

class Context : public core::NonCopyable<> {
public:
    Context() {
        ctx_ = roc_context_open(NULL);
        CHECK(ctx_);
        CHECK(roc_context_start(ctx_) == 0);
    }

    ~Context() {
        roc_context_stop(ctx_);
        roc_context_close(ctx_);
    }

    roc_context* get() {
        return ctx_;
    }

private:
    roc_context* ctx_;
};

class Sender : public core::Thread {
public:
    Sender(Context& context,
           roc_sender_config& config,
           const roc_address* dst_source_addr,
           const roc_address* dst_repair_addr,
           float* samples,
           size_t total_samples,
           size_t frame_size)
        : samples_(samples)
        , total_samples_(total_samples)
        , frame_size_(frame_size) {
        roc_address addr;
        CHECK(roc_address_init(&addr, ROC_AF_AUTO, "127.0.0.1", 0) == 0);
        sndr_ = roc_sender_open(context.get(), &config);
        CHECK(sndr_);
        CHECK(roc_sender_bind(sndr_, &addr) == 0);
        CHECK(roc_sender_connect(sndr_, ROC_PROTO_RTP_RSM8_SOURCE, dst_source_addr) == 0);
        CHECK(roc_sender_connect(sndr_, ROC_PROTO_RSM8_REPAIR, dst_repair_addr) == 0);
    }

    ~Sender() {
        roc_sender_close(sndr_);
    }

private:
    virtual void run() {
        for (size_t off = 0; off < total_samples_; off += frame_size_) {
            if (off + frame_size_ > total_samples_) {
                off = total_samples_ - frame_size_;
            }

            roc_frame frame;
            memset(&frame, 0, sizeof(frame));

            frame.samples = samples_ + off;
            frame.num_samples = frame_size_;

            const int ret = roc_sender_write(sndr_, &frame);
            roc_panic_if_not(ret == 0);
        }
    }

    roc_sender* sndr_;
    float* samples_;
    const size_t total_samples_;
    const size_t frame_size_;
};

class Receiver {
public:
    Receiver(Context& context,
             roc_receiver_config& config,
             const float* samples,
             size_t total_samples,
             size_t frame_size)
        : samples_(samples)
        , total_samples_(total_samples)
        , frame_size_(frame_size) {
        CHECK(roc_address_init(&source_addr_, ROC_AF_AUTO, "127.0.0.1", 0) == 0);
        CHECK(roc_address_init(&repair_addr_, ROC_AF_AUTO, "127.0.0.1", 0) == 0);
        recv_ = roc_receiver_open(context.get(), &config);
        CHECK(recv_);
        CHECK(roc_receiver_bind(recv_, ROC_PROTO_RTP_RSM8_SOURCE, &source_addr_) == 0);
        CHECK(roc_receiver_bind(recv_, ROC_PROTO_RSM8_REPAIR, &repair_addr_) == 0);
    }

    ~Receiver() {
        roc_receiver_close(recv_);
    }

    const roc_address* source_addr() const {
        return &source_addr_;
    }

    const roc_address* repair_addr() const {
        return &repair_addr_;
    }

    void run() {
        float rx_buff[MaxBufSize];

        size_t leading_zeros = 0;
        size_t sample_num = 0;
        size_t frame_num = 0;

        bool seek_first = true;
        bool finish = false;

        while (!finish) {
            size_t i = 0;
            frame_num++;

            roc_frame frame;
            memset(&frame, 0, sizeof(frame));

            frame.samples = rx_buff;
            frame.num_samples = frame_size_;

            roc_panic_if_not(roc_receiver_read(recv_, &frame) == 0);

            if (seek_first) {
                for (; i < frame_size_ && is_zero_(rx_buff[i]); i++, leading_zeros++) {
                }
                roc_panic_if_not(leading_zeros < Timeout);
                if (i < frame_size_) {
                    seek_first = false;
                }
            }

            if (!seek_first) {
                for (; i < frame_size_; i++) {
                    if (sample_num >= total_samples_) {
                        roc_panic_if_not(is_zero_(rx_buff[i]));
                        finish = true;
                        roc_log(LogDebug, "finish: leading_zeros: %lu, num_samples: %lu",
                                (unsigned long)leading_zeros, (unsigned long)sample_num);
                        break;
                    } else if (!is_zero_(samples_[sample_num] - rx_buff[i])) {
                        char sbuff[256];
                        int sbuff_i =
                            snprintf(sbuff, sizeof(sbuff),
                                     "failed comparing sample #%lu\n\nframe_num: %lu\n",
                                     (unsigned long)sample_num, (unsigned long)frame_num);
                        snprintf(&sbuff[sbuff_i], sizeof(sbuff) - (size_t)sbuff_i,
                                 "original: %f,\treceived: %f\n",
                                 (double)samples_[sample_num], (double)rx_buff[i]);
                        roc_panic("%s", sbuff);
                    } else {
                        sample_num++;
                    }
                }
            }
        }
    }

private:
    static inline bool is_zero_(float s) {
        return fabs(double(s)) < 1e-9;
    }

    roc_receiver* recv_;

    roc_address source_addr_;
    roc_address repair_addr_;

    const float* samples_;
    const size_t total_samples_;
    const size_t frame_size_;
};

class Proxy : private packet::IWriter {
public:
    Proxy(const roc_address* dst_source_addr,
          const roc_address* dst_repair_addr,
          size_t n_source_packets,
          size_t n_repair_packets)
        : trx_(packet_pool, byte_buffer_pool, allocator)
        , n_source_packets_(n_source_packets)
        , n_repair_packets_(n_repair_packets)
        , pos_(0) {
        dst_source_addr_.set_ipv4("127.0.0.1", roc_address_port(dst_source_addr));
        dst_repair_addr_.set_ipv4("127.0.0.1", roc_address_port(dst_repair_addr));
        send_addr_.set_ipv4("127.0.0.1", 0);
        recv_source_addr_.set_ipv4("127.0.0.1", 0);
        recv_repair_addr_.set_ipv4("127.0.0.1", 0);
        writer_ = trx_.add_udp_sender(send_addr_);
        CHECK(writer_);
        CHECK(trx_.add_udp_receiver(recv_source_addr_, *this));
        CHECK(trx_.add_udp_receiver(recv_repair_addr_, *this));
        CHECK(roc_address_init(&roc_source_addr_, ROC_AF_AUTO, "127.0.0.1",
                               recv_source_addr_.port())
              == 0);
        CHECK(roc_address_init(&roc_repair_addr_, ROC_AF_AUTO, "127.0.0.1",
                               recv_repair_addr_.port())
              == 0);
    }

    ~Proxy() {
        trx_.remove_port(send_addr_);
        trx_.remove_port(recv_source_addr_);
        trx_.remove_port(recv_repair_addr_);
    }

    const roc_address* source_addr() const {
        return &roc_source_addr_;
    }

    const roc_address* repair_addr() const {
        return &roc_repair_addr_;
    }

    void start() {
        trx_.start();
    }

    void stop() {
        trx_.stop();
        trx_.join();
    }

private:
    virtual void write(const packet::PacketPtr& pp) {
        pp->udp()->src_addr = send_addr_;

        if (pp->udp()->dst_addr == recv_source_addr_) {
            pp->udp()->dst_addr = dst_source_addr_;
            source_queue_.write(pp);
        } else {
            pp->udp()->dst_addr = dst_repair_addr_;
            repair_queue_.write(pp);
        }

        for (;;) {
            const size_t block_pos = pos_ % (n_source_packets_ + n_repair_packets_);

            if (block_pos < n_source_packets_) {
                if (!send_packet_(source_queue_, block_pos == 1)) {
                    return;
                }
            } else {
                if (!send_packet_(repair_queue_, false)) {
                    return;
                }
            }
        }
    }

    bool send_packet_(packet::IReader& reader, bool drop) {
        packet::PacketPtr pp = reader.read();
        if (!pp) {
            return false;
        }
        pos_++;
        if (!drop) {
            writer_->write(pp);
        }
        return true;
    }

    netio::Transceiver trx_;

    packet::Address send_addr_;

    roc_address roc_source_addr_;
    roc_address roc_repair_addr_;

    packet::Address recv_source_addr_;
    packet::Address recv_repair_addr_;

    packet::Address dst_source_addr_;
    packet::Address dst_repair_addr_;

    packet::Queue source_queue_;
    packet::Queue repair_queue_;

    packet::IWriter* writer_;

    const size_t n_source_packets_;
    const size_t n_repair_packets_;

    size_t pos_;
};

} // namespace

TEST_GROUP(sender_receiver) {
    roc_sender_config sender_conf;
    roc_receiver_config receiver_conf;

    float samples[TotalSamples];

    void setup() {
        roc_log_set_level((roc_log_level)core::Logger::instance().level());

        init_config();
        init_samples();
    }

    void init_config() {
        memset(&sender_conf, 0, sizeof(sender_conf));
        sender_conf.flags |= ROC_FLAG_ENABLE_TIMER;
        sender_conf.samples_per_packet = (unsigned int)PacketSamples / NumChans;
        sender_conf.fec_scheme = ROC_FEC_RS8M;
        sender_conf.n_source_packets = SourcePackets;
        sender_conf.n_repair_packets = RepairPackets;

        memset(&receiver_conf, 0, sizeof(receiver_conf));
        receiver_conf.flags |= ROC_FLAG_DISABLE_RESAMPLER;
        receiver_conf.flags |= ROC_FLAG_ENABLE_TIMER;
        receiver_conf.samples_per_packet = (unsigned int)PacketSamples / NumChans;
        receiver_conf.fec_scheme = ROC_FEC_RS8M;
        receiver_conf.n_source_packets = SourcePackets;
        receiver_conf.n_repair_packets = RepairPackets;
        receiver_conf.latency = Latency;
        receiver_conf.timeout = Timeout;
    }

    void init_samples() {
        const float sstep = 1. / 32768.;
        float sval = -1 + sstep;
        for (size_t i = 0; i < TotalSamples; ++i) {
            samples[i] = sval;
            sval += sstep;
            if (sval >= 1) {
                sval = -1 + sstep;
            }
        }
    }
};

TEST(sender_receiver, simple) {
    Context context;

    Receiver receiver(context, receiver_conf, samples, TotalSamples, FrameSamples);

    Sender sender(context, sender_conf, receiver.source_addr(), receiver.repair_addr(),
                  samples, TotalSamples, FrameSamples);

    sender.start();
    receiver.run();
    sender.join();
}

#ifdef ROC_TARGET_OPENFEC
TEST(sender_receiver, losses) {
    Context context;

    Receiver receiver(context, receiver_conf, samples, TotalSamples, FrameSamples);

    Proxy proxy(receiver.source_addr(), receiver.repair_addr(), SourcePackets,
                RepairPackets);

    Sender sender(context, sender_conf, proxy.source_addr(), proxy.repair_addr(), samples,
                  TotalSamples, FrameSamples);

    proxy.start();

    sender.start();
    receiver.run();
    sender.join();

    proxy.stop();
}
#endif // ROC_TARGET_OPENFEC

} // namespace roc
