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

        //! Check if there were any parsing errors.
        bool error() const;

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
        bool remove_padding_();
        bool check_sr_();
        bool check_rr_();

        State state_;
        const core::Slice<uint8_t> buf_;
        size_t cur_pos_;
        const header::PacketHeader* cur_pkt_header_;
        size_t cur_pkt_len_;
        core::Slice<uint8_t> cur_pkt_slice_;
        bool error_;
    };

    //! Initialize traverser.
    //! It will parse and iterate provided buffer.
    explicit Traverser(const core::Slice<uint8_t>& buf);

    //! Parse packet from buffer.
    bool parse();

    //! Construct iterator.
    //! @pre Can be used if parse() returned true.
    Iterator iter() const;

private:
    const core::Slice<uint8_t> buf_;
    bool parsed_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_TRAVERSER_H_
