/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/sdes_traverser.h
//! @brief Sdes enums.

#ifndef ROC_RTCP_SDES_TRAVERSER_H_
#define ROC_RTCP_SDES_TRAVERSER_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/sdes.h"

namespace roc {
namespace rtcp {

//! SDES packet traverer.
class SdesTraverser {
public:
    //! Packet iterator.
    class Iterator {
    public:
        //! Iterator state.
        enum State {
            BEGIN, //!< Iterator created.
            CHUNK, //!< SDES chunk.
            ITEM,  //!< SDES item.
            END    //!< Parsed whole packet.
        };

        //! Advance iterator.
        State next();

        //! Check if there were any parsing errors.
        bool error() const;

        //! Get SDES chunk.
        //! @pre Can be used if next() returned CHUNK.
        SdesChunk get_chunk() const;

        //! Get SDES item.
        //! Item is valid only until next() call.
        //! @pre Can be used if next() returned ITEM.
        SdesItem get_item() const;

    private:
        friend class SdesTraverser;

        explicit Iterator(const SdesTraverser& traverser);
        void next_element_();
        void parse_chunk_();
        void parse_item_();

        const SdesTraverser& traverser_;

        State state_;
        const core::Slice<uint8_t> buf_;
        size_t cur_pos_;
        size_t cur_chunk_;
        const header::SdesItemHeader* cur_item_header_;
        size_t cur_item_len_;
        bool error_;

        packet::stream_source_t parsed_ssrc_;
        header::SdesItemType parsed_item_type_;
        char parsed_item_text_[header::SdesItemHeader::MaxTextLen + 1];
    };

    //! Initialize traverser.
    //! It will parse and iterate provided buffer.
    explicit SdesTraverser(const core::Slice<uint8_t>& buf);

    //! Parse packet from buffer.
    bool parse();

    //! Construct iterator.
    //! @pre Can be used if parse() returned true.
    Iterator iter() const;

    //! Get number of SDES chunks in packet.
    size_t chunks_count() const;

private:
    const core::Slice<uint8_t> buf_;
    bool parsed_;
    size_t packet_len_;
    size_t chunks_count_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_SDES_TRAVERSER_H_
