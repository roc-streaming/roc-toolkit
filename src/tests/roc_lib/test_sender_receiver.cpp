/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
#include "roc_core/random.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/parse_address.h"

#include "roc/receiver.h"
#include "roc/sender.h"

namespace roc {

namespace {

enum { MaxBufSize = 4096 };

core::HeapAllocator allocator;
packet::PacketPool packet_pool(allocator, 1);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, 1);

class Sender : public core::Thread {
public:
    Sender(roc_sender_config& config,
           packet::Address dst_source_addr,
           packet::Address dst_repair_addr,
           float* samples,
           size_t len,
           size_t frame_size)
        : samples_(samples)
        , sz_(len)
        , frame_size_(frame_size) {
        packet::Address addr;
        CHECK(packet::parse_address("127.0.0.1:0", addr));
        sndr_ = roc_sender_new(&config);
        CHECK(sndr_);
        CHECK(roc_sender_bind(sndr_, addr.saddr()) == 0);
        CHECK(
            roc_sender_connect(sndr_, ROC_PROTO_RTP_RSM8_SOURCE, dst_source_addr.saddr())
            == 0);
        CHECK(roc_sender_connect(sndr_, ROC_PROTO_RSM8_REPAIR, dst_repair_addr.saddr())
              == 0);
        CHECK(roc_sender_start(sndr_) == 0);
    }

    ~Sender() {
        roc_sender_stop(sndr_);
        roc_sender_delete(sndr_);
    }

private:
    virtual void run() {
        for (size_t off = 0; off < sz_; off += frame_size_) {
            if (off + frame_size_ > sz_) {
                off = sz_ - frame_size_;
            }
            const ssize_t ret = roc_sender_write(sndr_, samples_ + off, frame_size_);
            LONGS_EQUAL(frame_size_, ret);
        }
    }

    roc_sender* sndr_;
    float* samples_;
    const size_t sz_;
    const size_t frame_size_;
};

class Receiver {
public:
    Receiver(roc_receiver_config& config) {
        CHECK(packet::parse_address("127.0.0.1:0", source_addr_));
        CHECK(packet::parse_address("127.0.0.1:0", repair_addr_));
        recv_ = roc_receiver_new(&config);
        CHECK(recv_);
        CHECK(roc_receiver_bind(recv_, ROC_PROTO_RTP_RSM8_SOURCE, source_addr_.saddr())
              == 0);
        CHECK(roc_receiver_bind(recv_, ROC_PROTO_RSM8_REPAIR, repair_addr_.saddr()) == 0);
        CHECK(roc_receiver_start(recv_) == 0);
    }

    ~Receiver() {
        roc_receiver_stop(recv_);
        roc_receiver_delete(recv_);
    }

    packet::Address source_addr() {
        return source_addr_;
    }

    packet::Address repair_addr() {
        return repair_addr_;
    }

    ssize_t read(float* samples, const size_t n_samples) {
        return roc_receiver_read(recv_, samples, n_samples);
    }

private:
    roc_receiver* recv_;

    packet::Address source_addr_;
    packet::Address repair_addr_;
};

class Proxy : private packet::IWriter {
public:
    Proxy(const packet::Address& dst_source_addr,
          const packet::Address& dst_repair_addr,
          const size_t block_size)
        : trx_(packet_pool, byte_buffer_pool, allocator)
        , dst_source_addr_(dst_source_addr)
        , dst_repair_addr_(dst_repair_addr)
        , block_size_(block_size)
        , num_(0) {
        CHECK(packet::parse_address("127.0.0.1:0", send_addr_));
        CHECK(packet::parse_address("127.0.0.1:0", recv_source_addr_));
        CHECK(packet::parse_address("127.0.0.1:0", recv_repair_addr_));
        writer_ = trx_.add_udp_sender(send_addr_);
        CHECK(writer_);
        CHECK(trx_.add_udp_receiver(recv_source_addr_, *this));
        CHECK(trx_.add_udp_receiver(recv_repair_addr_, *this));
    }

    packet::Address source_addr() {
        return recv_source_addr_;
    }

    packet::Address repair_addr() {
        return recv_repair_addr_;
    }

    void start() {
        trx_.start();
    }

    void stop() {
        trx_.stop();
        trx_.join();
    }

private:
    virtual void write(const packet::PacketPtr& ptr) {
        if (num_++ % block_size_ == 1) {
            return;
        }
        ptr->udp()->src_addr = send_addr_;
        if (ptr->udp()->dst_addr == recv_source_addr_) {
            ptr->udp()->dst_addr = dst_source_addr_;
        } else {
            ptr->udp()->dst_addr = dst_repair_addr_;
        }
        writer_->write(ptr);
    }

    netio::Transceiver trx_;

    packet::Address send_addr_;

    packet::Address recv_source_addr_;
    packet::Address recv_repair_addr_;

    packet::Address dst_source_addr_;
    packet::Address dst_repair_addr_;

    packet::IWriter* writer_;

    const size_t block_size_;
    size_t num_;
};

} // namespace

TEST_GROUP(sender_receiver) {
    static const size_t n_channels = 2;

    static const size_t n_source_packets = 10;
    static const size_t n_repair_packets = 5;

    static const size_t packet_len = 100;
    static const size_t packet_num = n_source_packets * 5;

    static const size_t frame_size = packet_len * 2;

    roc_sender_config sender_conf;
    roc_receiver_config receiver_conf;

    static const size_t total_sz = packet_len * packet_num;
    float s2send[total_sz];
    float s2recv[total_sz];

    void setup() {
        memset(&sender_conf, 0, sizeof(sender_conf));
        sender_conf.flags |= ROC_FLAG_DISABLE_INTERLEAVER;
        sender_conf.flags |= ROC_FLAG_ENABLE_TIMER;
        sender_conf.samples_per_packet = (unsigned int)packet_len / n_channels;
        sender_conf.fec_scheme = ROC_FEC_RS8M;
        sender_conf.n_source_packets = n_source_packets;
        sender_conf.n_repair_packets = n_repair_packets;

        memset(&receiver_conf, 0, sizeof(receiver_conf));
        receiver_conf.flags |= ROC_FLAG_DISABLE_RESAMPLER;
        receiver_conf.flags |= ROC_FLAG_ENABLE_TIMER;
        receiver_conf.samples_per_packet = (unsigned int)packet_len / n_channels;
        receiver_conf.fec_scheme = ROC_FEC_RS8M;
        receiver_conf.n_source_packets = n_source_packets;
        receiver_conf.n_repair_packets = n_repair_packets;
        receiver_conf.latency = packet_len * 20;
        receiver_conf.timeout = packet_len * 300;

        const float sstep = 1. / 32768.;
        float sval = -1 + sstep;
        for (size_t i = 0; i < total_sz; ++i) {
            s2send[i] = sval;
            sval += sstep;
            if (sval >= 1) {
                sval = -1 + sstep;
            }
        }
    }

    void check_sample_arrays(Receiver & recv, float* original, const size_t len) {
        float rx_buff[packet_len];
        size_t s_first = 0;
        size_t inner_cntr = 0;
        bool seek_first = true;
        size_t s_last = 0;

        size_t ipacket = 0;
        while (s_last == 0) {
            size_t i = 0;
            ipacket++;
            LONGS_EQUAL(packet_len, recv.read(rx_buff, packet_len));
            if (seek_first) {
                for (; i < packet_len && fabs(double(rx_buff[i])) < 1e-9;
                     i++, s_first++) {
                }
                CHECK(s_first < len);
                if (i < packet_len) {
                    seek_first = false;
                }
            }
            if (!seek_first) {
                for (; i < packet_len; i++) {
                    if (inner_cntr >= len) {
                        CHECK(fabs(double(rx_buff[i])) < 1e-9);
                        s_last = inner_cntr + s_first;
                        roc_log(LogInfo,
                                "finish: s_first: %lu, s_last: %lu, inner_cntr: %lu",
                                (unsigned long)s_first, (unsigned long)s_last,
                                (unsigned long)inner_cntr);
                        break;
                    } else if (fabs(double(original[inner_cntr] - rx_buff[i])) > 1e-9) {
                        char sbuff[256];
                        int sbuff_i =
                            snprintf(sbuff, sizeof(sbuff),
                                     "failed comparing samples #%lu\n\npacket_num: %lu\n",
                                     (unsigned long)inner_cntr, (unsigned long)ipacket);
                        snprintf(&sbuff[sbuff_i], sizeof(sbuff) - (size_t)sbuff_i,
                                 "original: %f,\treceived: %f\n",
                                 (double)original[inner_cntr], (double)rx_buff[i]);
                        FAIL(sbuff);
                    } else {
                        inner_cntr++;
                    }
                }
            }
        }
    }
};

TEST(sender_receiver, simple) {
    Receiver recv(receiver_conf);

    Sender sndr(sender_conf, recv.source_addr(), recv.repair_addr(), s2send, total_sz,
                frame_size);

    sndr.start();
    check_sample_arrays(recv, s2send, total_sz);
    sndr.join();
}

#ifdef ROC_TARGET_OPENFEC
TEST(sender_receiver, losses) {
    Receiver recv(receiver_conf);

    Proxy proxy(recv.source_addr(), recv.repair_addr(),
                n_source_packets + n_repair_packets);

    Sender sndr(sender_conf, proxy.source_addr(), proxy.repair_addr(), s2send, total_sz,
                frame_size);

    proxy.start();

    sndr.start();
    check_sample_arrays(recv, s2send, total_sz);
    sndr.join();

    proxy.stop();
}
#endif // ROC_TARGET_OPENFEC

} // namespace roc
