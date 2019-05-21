/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_FEC_TARGET_OPENFEC_TEST_PACKET_DISPATCHER_H_
#define ROC_FEC_TARGET_OPENFEC_TEST_PACKET_DISPATCHER_H_

#include "roc_core/helpers.h"
#include "roc_packet/fec.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/sorted_queue.h"

namespace roc {
namespace fec {

// Divides packets from Encoder into two queues: source and repair packets,
// as needed for Decoder.
class PacketDispatcher : public packet::IWriter {
public:
    PacketDispatcher(size_t num_source, size_t num_repair)
        : num_source_(num_source)
        , num_repair_(num_repair)
        , packet_num_(0)
        , source_queue_(0)
        , source_stock_(0)
        , repair_queue_(0)
        , repair_stock_(0)
        , n_lost_(0)
        , n_delayed_(0) {
        reset();
    }

    virtual void write(const packet::PacketPtr& p) {
        write_(p);

        if (++packet_num_ >= num_source_ + num_repair_) {
            packet_num_ = 0;
        }
    }

    packet::IReader& source_reader() {
        return source_queue_;
    }

    packet::IReader& repair_reader() {
        return repair_queue_;
    }

    size_t source_size() {
        return source_stock_.size() + source_queue_.size();
    }

    size_t repair_size() {
        return repair_stock_.size() + repair_queue_.size();
    }

    packet::PacketPtr repair_head() {
        return repair_queue_.head();
    }

    void reset() {
        const size_t n_source_packets = source_queue_.size();
        const size_t n_repair_packets = repair_queue_.size();

        for (size_t i = 0; i < n_source_packets; ++i) {
            source_queue_.read();
        }

        for (size_t i = 0; i < n_repair_packets; ++i) {
            repair_queue_.read();
        }

        packet_num_ = 0;

        clear_losses();
        clear_delays();
    }

    void lose(const size_t n) {
        CHECK(n_lost_ != MaxLost);
        lost_packet_nums_[n_lost_++] = n;
    }

    void clear_losses() {
        n_lost_ = 0;
    }

    void delay(const size_t n) {
        CHECK(n_delayed_ != MaxDelayed);
        delayed_packet_nums_[n_delayed_++] = n;
    }

    void clear_delays() {
        for (size_t i = 0; i < MaxDelayed; i++) {
            delayed_stock_[i] = NULL;
        }
        n_delayed_ = 0;
    }

    void push_written() {
        while (source_stock_.head()) {
            source_queue_.write(source_stock_.read());
        }
        while (repair_stock_.head()) {
            repair_queue_.write(repair_stock_.read());
        }
    }

    bool push_one_source() {
        packet::PacketPtr p;
        if (!(p = source_stock_.read())) {
            return false;
        }
        source_queue_.write(p);
        return true;
    }

    void push_delayed(const size_t n) {
        for (size_t i = 0; i < n_delayed_; i++) {
            if (delayed_packet_nums_[i] == n) {
                if (delayed_stock_[i]) {
                    route_(source_queue_, repair_queue_, delayed_stock_[i]);
                    delayed_stock_[i] = NULL;
                } else {
                    FAIL("no delayed packet");
                }
            }
        }
    }

private:
    void write_(const packet::PacketPtr& p) {
        if (is_lost_(packet_num_)) {
            return;
        }

        if (delay_(packet_num_, p)) {
            return;
        }

        route_(source_stock_, repair_stock_, p);
    }

    void
    route_(packet::SortedQueue& sq, packet::SortedQueue& rq, const packet::PacketPtr& p) {
        if (p->flags() & packet::Packet::FlagAudio) {
            sq.write(p);
        } else if (p->flags() & packet::Packet::FlagRepair) {
            rq.write(p);
        } else {
            FAIL("unexpected packet type");
        }
    }

    bool is_lost_(size_t n) const {
        for (size_t i = 0; i < n_lost_; i++) {
            if (lost_packet_nums_[i] == n) {
                return true;
            }
        }
        return false;
    }

    bool delay_(size_t n, packet::PacketPtr pp) {
        for (size_t i = 0; i < n_delayed_; i++) {
            if (delayed_packet_nums_[i] == n) {
                delayed_stock_[i] = pp;
                return true;
            }
        }
        return false;
    }

    const size_t num_source_;
    const size_t num_repair_;

    size_t packet_num_;

    packet::SortedQueue source_queue_;
    packet::SortedQueue source_stock_;

    packet::SortedQueue repair_queue_;
    packet::SortedQueue repair_stock_;

    enum { MaxLost = 100 };

    size_t lost_packet_nums_[MaxLost];
    size_t n_lost_;

    enum { MaxDelayed = 100 };

    size_t delayed_packet_nums_[MaxDelayed];
    size_t n_delayed_;

    packet::PacketPtr delayed_stock_[MaxDelayed];
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_TARGET_OPENFEC_TEST_PACKET_DISPATCHER_H_
