/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/sdes.h
//! @brief SDES elements.

#ifndef ROC_RTCP_SDES_H_
#define ROC_RTCP_SDES_H_

#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

//! Parsed SDES chunk.
struct SdesChunk {
    SdesChunk()
        : ssrc(0) {
    }

    //! Source ID.
    packet::stream_source_t ssrc;
};

//! Parsed SDES item.
struct SdesItem {
    SdesItem()
        : type(header::SdesItemType(0))
        , text(NULL) {
    }

    //! Item type.
    header::SdesItemType type;

    //! Item text.
    //! Zero-terminated UTF-8 string.
    const char* text;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_SDES_H_
