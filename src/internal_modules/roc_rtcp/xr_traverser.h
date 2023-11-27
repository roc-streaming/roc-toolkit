/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/xr_traverser.h
//! @brief XR Traverser.

#ifndef ROC_RTCP_XR_TRAVERSER_H_
#define ROC_RTCP_XR_TRAVERSER_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

//! XR packet traverser.
class XrTraverser {
public:
    //! Packet iterator.
    class Iterator {
    public:
        //! Iterator state.
        enum State {
            BEGIN,      //!< Iterator created.
            RRTR_BLOCK, //!< RRTR block (receiver reference time).
            DLRR_BLOCK, //!< DLRR block (delay since last receiver report).
            END         //!< Parsed whole packet.
        };

        //! Advance iterator.
        State next();

        //! Check if there were any parsing errors.
        bool error() const;

        //! Get RRTR block (receiver reference time).
        //! @pre Can be used if next() returned RRTR_BLOCK.
        const header::XrRrtrBlock& get_rrtr() const;

        //! Get DLRR block (delay since last receiver report).
        //! @pre Can be used if next() returned DLRR_BLOCK.
        const header::XrDlrrBlock& get_dlrr() const;

    private:
        friend class XrTraverser;

        explicit Iterator(const XrTraverser& traverser);
        void next_block_();
        bool check_rrtr_();
        bool check_dlrr_();

        State state_;
        const core::Slice<uint8_t> buf_;
        size_t cur_pos_;
        const header::XrBlockHeader* cur_blk_header_;
        size_t cur_blk_len_;
        bool error_;
    };

    //! Initialize traverser.
    //! It will parse and iterate provided buffer.
    explicit XrTraverser(const core::Slice<uint8_t>& buf);

    //! Parse packet from buffer.
    bool parse();

    //! Construct iterator.
    //! @pre Can be used if parse() returned true.
    Iterator iter() const;

    //! Get number of XR blocks in packet.
    size_t blocks_count() const;

    //! Get XR packet.
    const header::XrPacket& packet() const;

private:
    core::Slice<uint8_t> buf_;
    bool parsed_;
    size_t blocks_count_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_XR_TRAVERSER_H_
