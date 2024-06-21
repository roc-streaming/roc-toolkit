/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_CONTROL_READER_H_
#define ROC_PIPELINE_TEST_HELPERS_CONTROL_READER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_address/socket_addr.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"
#include "roc_rtcp/print_packet.h"
#include "roc_rtcp/traverser.h"

namespace roc {
namespace pipeline {
namespace test {

// Generates control packets and pass them to destination reader
class ControlReader : public core::NonCopyable<> {
public:
    ControlReader(packet::IReader& reader)
        : reader_(reader) {
    }

    void read_report() {
        LONGS_EQUAL(status::StatusOK, reader_.read(packet_, packet::ModeFetch));
        CHECK(packet_);

        CHECK(packet_->flags() & packet::Packet::FlagUDP);
        CHECK(packet_->flags() & packet::Packet::FlagRTCP);
        CHECK(packet_->flags() & packet::Packet::FlagComposed);

        if (core::Logger::instance().get_level() >= LogTrace) {
            rtcp::print_packet(packet_->rtcp()->payload);
        }
    }

    const address::SocketAddr& dst_addr() const {
        CHECK(packet_);
        CHECK(packet_->udp());

        return packet_->udp()->dst_addr;
    }

    bool has_src_addr(const address::SocketAddr& addr = address::SocketAddr()) {
        CHECK(packet_);
        CHECK(packet_->udp());

        if (addr) {
            return packet_->udp()->src_addr == addr;
        } else {
            return packet_->udp()->src_addr;
        }
    }

    bool has_dst_addr(const address::SocketAddr& addr = address::SocketAddr()) {
        CHECK(packet_);
        CHECK(packet_->udp());

        if (addr) {
            return packet_->udp()->dst_addr == addr;
        } else {
            return packet_->udp()->dst_addr;
        }
    }

    bool has_sr(packet::stream_source_t from = 0) const {
        CHECK(packet_);
        CHECK(packet_->rtcp());

        rtcp::Traverser traverser(packet_->rtcp()->payload);
        CHECK(traverser.parse());

        rtcp::Traverser::Iterator iter = traverser.iter();
        rtcp::Traverser::Iterator::State state;

        while ((state = iter.next()) != rtcp::Traverser::Iterator::END) {
            switch (state) {
            case rtcp::Traverser::Iterator::SR:
                if (iter.get_sr().ssrc() == from || from == 0) {
                    return true;
                }
                break;

            default:
                break;
            }
        }

        return false;
    }

    bool has_rr(packet::stream_source_t from = 0, packet::stream_source_t to = 0) const {
        CHECK(packet_);
        CHECK(packet_->rtcp());

        rtcp::Traverser traverser(packet_->rtcp()->payload);
        CHECK(traverser.parse());

        rtcp::Traverser::Iterator iter = traverser.iter();
        rtcp::Traverser::Iterator::State state;

        while ((state = iter.next()) != rtcp::Traverser::Iterator::END) {
            switch (state) {
            case rtcp::Traverser::Iterator::RR:
                if (iter.get_rr().ssrc() == from || from == 0) {
                    for (size_t n = 0; n < iter.get_rr().num_blocks(); n++) {
                        if (iter.get_rr().get_block(n).ssrc() == to || to == 0) {
                            return true;
                        }
                    }
                }
                break;

            default:
                break;
            }
        }

        return false;
    }

    bool has_rrtr(packet::stream_source_t from = 0) const {
        CHECK(packet_);
        CHECK(packet_->rtcp());

        rtcp::Traverser traverser(packet_->rtcp()->payload);
        CHECK(traverser.parse());

        rtcp::Traverser::Iterator iter = traverser.iter();
        rtcp::Traverser::Iterator::State state;

        while ((state = iter.next()) != rtcp::Traverser::Iterator::END) {
            switch (state) {
            case rtcp::Traverser::Iterator::XR: {
                rtcp::XrTraverser xr = iter.get_xr();
                CHECK(xr.parse());

                if (xr.packet().ssrc() == from || from == 0) {
                    rtcp::XrTraverser::Iterator xr_iter = xr.iter();
                    rtcp::XrTraverser::Iterator::Iterator::State xr_state;

                    while ((xr_state = xr_iter.next())
                           != rtcp::XrTraverser::Iterator::END) {
                        switch (xr_state) {
                        case rtcp::XrTraverser::Iterator::RRTR_BLOCK:
                            return true;

                        default:
                            break;
                        }
                    }
                }
            } break;

            default:
                break;
            }
        }

        return false;
    }

    bool has_dlrr(packet::stream_source_t from = 0,
                  packet::stream_source_t to = 0) const {
        CHECK(packet_);
        CHECK(packet_->rtcp());

        rtcp::Traverser traverser(packet_->rtcp()->payload);
        CHECK(traverser.parse());

        rtcp::Traverser::Iterator iter = traverser.iter();
        rtcp::Traverser::Iterator::State state;

        while ((state = iter.next()) != rtcp::Traverser::Iterator::END) {
            switch (state) {
            case rtcp::Traverser::Iterator::XR: {
                rtcp::XrTraverser xr = iter.get_xr();
                CHECK(xr.parse());

                if (xr.packet().ssrc() == from || from == 0) {
                    rtcp::XrTraverser::Iterator xr_iter = xr.iter();
                    rtcp::XrTraverser::Iterator::Iterator::State xr_state;

                    while ((xr_state = xr_iter.next())
                           != rtcp::XrTraverser::Iterator::END) {
                        switch (xr_state) {
                        case rtcp::XrTraverser::Iterator::DLRR_BLOCK:
                            for (size_t n = 0; n < xr_iter.get_dlrr().num_subblocks();
                                 n++) {
                                if (xr_iter.get_dlrr().get_subblock(n).ssrc() == to
                                    || to == 0) {
                                    return true;
                                }
                            }
                            break;

                        default:
                            break;
                        }
                    }
                }
            } break;

            default:
                break;
            }
        }

        return false;
    }

    bool has_measurement_info(packet::stream_source_t from = 0) const {
        CHECK(packet_);
        CHECK(packet_->rtcp());

        rtcp::Traverser traverser(packet_->rtcp()->payload);
        CHECK(traverser.parse());

        rtcp::Traverser::Iterator iter = traverser.iter();
        rtcp::Traverser::Iterator::State state;

        while ((state = iter.next()) != rtcp::Traverser::Iterator::END) {
            switch (state) {
            case rtcp::Traverser::Iterator::XR: {
                rtcp::XrTraverser xr = iter.get_xr();
                CHECK(xr.parse());

                if (xr.packet().ssrc() == from || from == 0) {
                    rtcp::XrTraverser::Iterator xr_iter = xr.iter();
                    rtcp::XrTraverser::Iterator::Iterator::State xr_state;

                    while ((xr_state = xr_iter.next())
                           != rtcp::XrTraverser::Iterator::END) {
                        switch (xr_state) {
                        case rtcp::XrTraverser::Iterator::MEASUREMENT_INFO_BLOCK:
                            return true;

                        default:
                            break;
                        }
                    }
                }
            } break;

            default:
                break;
            }
        }

        return false;
    }

    bool has_delay_metrics(packet::stream_source_t from = 0) const {
        CHECK(packet_);
        CHECK(packet_->rtcp());

        rtcp::Traverser traverser(packet_->rtcp()->payload);
        CHECK(traverser.parse());

        rtcp::Traverser::Iterator iter = traverser.iter();
        rtcp::Traverser::Iterator::State state;

        while ((state = iter.next()) != rtcp::Traverser::Iterator::END) {
            switch (state) {
            case rtcp::Traverser::Iterator::XR: {
                rtcp::XrTraverser xr = iter.get_xr();
                CHECK(xr.parse());

                if (xr.packet().ssrc() == from || from == 0) {
                    rtcp::XrTraverser::Iterator xr_iter = xr.iter();
                    rtcp::XrTraverser::Iterator::Iterator::State xr_state;

                    while ((xr_state = xr_iter.next())
                           != rtcp::XrTraverser::Iterator::END) {
                        switch (xr_state) {
                        case rtcp::XrTraverser::Iterator::DELAY_METRICS_BLOCK:
                            return true;

                        default:
                            break;
                        }
                    }
                }
            } break;

            default:
                break;
            }
        }

        return false;
    }

    bool has_queue_metrics(packet::stream_source_t from = 0) const {
        CHECK(packet_);
        CHECK(packet_->rtcp());

        rtcp::Traverser traverser(packet_->rtcp()->payload);
        CHECK(traverser.parse());

        rtcp::Traverser::Iterator iter = traverser.iter();
        rtcp::Traverser::Iterator::State state;

        while ((state = iter.next()) != rtcp::Traverser::Iterator::END) {
            switch (state) {
            case rtcp::Traverser::Iterator::XR: {
                rtcp::XrTraverser xr = iter.get_xr();
                CHECK(xr.parse());

                if (xr.packet().ssrc() == from || from == 0) {
                    rtcp::XrTraverser::Iterator xr_iter = xr.iter();
                    rtcp::XrTraverser::Iterator::Iterator::State xr_state;

                    while ((xr_state = xr_iter.next())
                           != rtcp::XrTraverser::Iterator::END) {
                        switch (xr_state) {
                        case rtcp::XrTraverser::Iterator::QUEUE_METRICS_BLOCK:
                            return true;

                        default:
                            break;
                        }
                    }
                }
            } break;

            default:
                break;
            }
        }

        return false;
    }

private:
    packet::IReader& reader_;
    packet::PacketPtr packet_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_CONTROL_READER_H_
