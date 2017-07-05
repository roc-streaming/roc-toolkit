/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/format_map.h
//! @brief RTP payload format map.

#ifndef ROC_RTP_FORMAT_MAP_H_
#define ROC_RTP_FORMAT_MAP_H_

#include "roc_core/noncopyable.h"
#include "roc_rtp/format.h"

namespace roc {
namespace rtp {

//! RTP payload format map.
class FormatMap : public core::NonCopyable<> {
public:
    //! Get format by payload type.
    //! @returns
    //!  pointer to the format structure or null if there is no format
    //!  registered for this payload type.
    const Format* format(unsigned int pt) const;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_FORMAT_MAP_H_
