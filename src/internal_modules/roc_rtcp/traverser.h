/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/traverser.h
//! @brief RTCP packet traverser.

#ifndef ROC_RTCP_TRAVERSER_H_
#define ROC_RTCP_TRAVERSER_H_

#include "roc_rtcp/bye_traverser.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/sdes_traverser.h"
#include "roc_rtcp/xr_traverser.h"

namespace roc {
namespace rtcp {

//! RTCP compound packet traverser.
class Traverser {
public:
    //! Packet iterator.
    class Iterator {
    public:
        //! Iterator state.
        enum State {
            BEGIN, //!< Iterator created.
            SR,    //!< SR packet.
            RR,    //!< RR packet.
            XR,    //!< XR packet.
            SDES,  //!< SDES packet.
            BYE,   //!< BYE packet.
            END    //!< Parsed whole compound packet.
        };

        //! Advance iterator.
        State next();

        //! Get SR packet.
        //! @pre Can be used if next() returned SR.
        const header::SenderReportPacket& get_sr() const;

        //! Get RR packet.
        //! @pre Can be used if next() returned RR.
        const header::ReceiverReportPacket& get_rr() const;

        //! Get traverser for XR packet.
        //! @pre Can be used if next() returned XR.
        XrTraverser get_xr() const;

        //! Get traverser for SDES packet.
        //! @pre Can be used if next() returned SDES.
        SdesTraverser get_sdes();

        //! Get traverser for BYE packet.
        //! @pre Can be used if next() returned BYE.
        ByeTraverser get_bye();

    private:
        friend class Traverser;

        explicit Iterator(const Traverser& traverser);
        void next_packet_();
        void skip_packet_();

        State state_;
        const core::Slice<uint8_t> data_;
        core::Slice<uint8_t> cur_slice_;
        header::PacketHeader* cur_pkt_header_;
        size_t cur_pkt_len_;
        size_t cur_i_;
    };

    //! Initialize traverser.
    //! It will parse and iterate provided buffer.
    explicit Traverser(const core::Slice<uint8_t>& data);

    //! Validate packet for strict correctness according to the RFC.
    bool validate() const;

    //! Parse packet from buffer.
    bool parse();

    //! Construct iterator.
    //! @pre Can be used if parse() returned true.
    Iterator iter() const;

private:
    core::Slice<uint8_t> data_;
    bool parsed_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_TRAVERSER_H_
