/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/bye_traverser.h
//! @brief RTCP interface structures.

#ifndef ROC_RTCP_BYE_TRAVERSER_H_
#define ROC_RTCP_BYE_TRAVERSER_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

//! BYE packet traverer.
class ByeTraverser {
public:
    //! Packet iterator.
    class Iterator {
    public:
        //! Iterator state.
        enum State {
            BEGIN,  //!< Iterator created.
            SSRC,   //!< SSRC element.
            REASON, //!< REASON element.
            END     //!< Parsed whole packet.
        };

        //! Advance iterator.
        State next();

        //! Check if there were any parsing errors.
        bool error() const;

        //! Get SSRC element.
        //! @pre Can be used if next() returned SSRC.
        packet::stream_source_t get_ssrc() const;

        //! Get REASON element.
        //! Zero-terminated UTF-8 string.
        //! Pointer is valid only until next() call.
        //! @pre Can be used if next() returned REASON.
        const char* get_reason() const;

    private:
        friend class ByeTraverser;

        explicit Iterator(const ByeTraverser& traverser);
        void next_element_();
        void parse_ssrc_();
        void parse_reason_();

        const ByeTraverser& traverser_;

        State state_;
        const core::Slice<uint8_t> buf_;
        size_t cur_pos_;
        size_t cur_ssrc_;
        bool error_;

        packet::stream_source_t parsed_ssrc_;
        char parsed_reason_[header::ByeReasonHeader::MaxTextLen + 1];
    };

    //! Initialize traverser.
    //! It will parse and iterate provided buffer.
    explicit ByeTraverser(const core::Slice<uint8_t>& buf);

    //! Parse packet from buffer.
    bool parse();

    //! Construct iterator.
    //! @pre Can be used if parse() returned true.
    Iterator iter() const;

    //! Get number of SSRC elements in packet.
    size_t ssrc_count() const;

private:
    const core::Slice<uint8_t> buf_;
    bool parsed_;
    size_t packet_len_;
    size_t ssrc_count_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_BYE_TRAVERSER_H_
