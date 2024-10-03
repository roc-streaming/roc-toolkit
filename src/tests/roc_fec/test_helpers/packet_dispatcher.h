/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_FEC_TEST_HELPERS_PACKET_DISPATCHER_H_
#define ROC_FEC_TEST_HELPERS_PACKET_DISPATCHER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/macro_helpers.h"
#include "roc_packet/fec.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/sorted_queue.h"

namespace roc {
namespace fec {
namespace test {

// Divides packets from Encoder into two queues: source and repair packets,
// as needed for Decoder.
class PacketDispatcher : public packet::IWriter {
public:
    PacketDispatcher(packet::IParser& source_parser,
                     packet::IParser& repair_parser,
                     packet::PacketFactory& packet_factory,
                     size_t num_source,
                     size_t num_repair)
        : source_parser_(source_parser)
        , repair_parser_(repair_parser)
        , packet_factory_(packet_factory)
        , num_source_(num_source)
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

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr& p) {
        store_(p);

        if (++packet_num_ >= num_source_ + num_repair_) {
            packet_num_ = 0;
        }

        return status::StatusOK;
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

    void resize(size_t num_source, size_t num_repair) {
        num_source_ = num_source;
        num_repair_ = num_repair;
    }

    void reset() {
        const size_t n_source_packets = source_queue_.size();
        const size_t n_repair_packets = repair_queue_.size();

        for (size_t i = 0; i < n_source_packets; ++i) {
            packet::PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, source_queue_.read(pp, packet::ModeFetch));
            CHECK(pp);
        }

        for (size_t i = 0; i < n_repair_packets; ++i) {
            packet::PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, repair_queue_.read(pp, packet::ModeFetch));
            CHECK(pp);
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

    void push_stocks() {
        while (source_stock_.head()) {
            packet::PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, source_stock_.read(pp, packet::ModeFetch));
            deliver_(pp);
        }
        while (repair_stock_.head()) {
            packet::PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, repair_stock_.read(pp, packet::ModeFetch));
            deliver_(pp);
        }
    }

    void push_source_stock(size_t limit) {
        for (size_t n = 0; n < limit; n++) {
            packet::PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, source_stock_.read(pp, packet::ModeFetch));
            deliver_(pp);
        }
    }

    void push_repair_stock(size_t limit) {
        for (size_t n = 0; n < limit; n++) {
            packet::PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, repair_stock_.read(pp, packet::ModeFetch));
            deliver_(pp);
        }
    }

    void push_delayed(const size_t index) {
        for (size_t i = 0; i < n_delayed_; i++) {
            if (delayed_packet_nums_[i] == index) {
                if (delayed_stock_[i]) {
                    deliver_(delayed_stock_[i]);
                    delayed_stock_[i] = NULL;
                } else {
                    FAIL("no delayed packet");
                }
            }
        }
    }

private:
    void store_(const packet::PacketPtr& p) {
        CHECK(p);

        if (is_lost_(packet_num_)) {
            return;
        }

        if (delay_(packet_num_, p)) {
            return;
        }

        if (p->flags() & packet::Packet::FlagAudio) {
            LONGS_EQUAL(status::StatusOK, source_stock_.write(p));
        } else if (p->flags() & packet::Packet::FlagRepair) {
            LONGS_EQUAL(status::StatusOK, repair_stock_.write(p));
        } else {
            FAIL("unexpected packet type");
        }
    }

    void deliver_(const packet::PacketPtr& p) {
        CHECK(p);

        if (p->flags() & packet::Packet::FlagAudio) {
            LONGS_EQUAL(status::StatusOK,
                        source_queue_.write(reparse_packet_(source_parser_, p)));
        } else if (p->flags() & packet::Packet::FlagRepair) {
            LONGS_EQUAL(status::StatusOK,
                        repair_queue_.write(reparse_packet_(repair_parser_, p)));
        } else {
            FAIL("unexpected packet type");
        }
    }

    packet::PacketPtr reparse_packet_(packet::IParser& parser,
                                      const packet::PacketPtr& old_pp) {
        CHECK(old_pp);
        CHECK(old_pp->flags() & packet::Packet::FlagComposed);

        packet::PacketPtr pp = packet_factory_.new_packet();
        if (!pp) {
            FAIL("can't allocate packet");
        }

        if (parser.parse(*pp, old_pp->buffer()) != status::StatusOK) {
            FAIL("can't parse packet");
        }

        pp->set_buffer(old_pp->buffer());

        return pp;
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

    packet::IParser& source_parser_;
    packet::IParser& repair_parser_;
    packet::PacketFactory& packet_factory_;

    size_t num_source_;
    size_t num_repair_;

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

} // namespace test
} // namespace fec
} // namespace roc

#endif // ROC_FEC_TEST_HELPERS_PACKET_DISPATCHER_H_
