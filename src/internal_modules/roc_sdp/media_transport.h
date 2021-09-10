/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/media_transport.h
//! @brief SDP media transport protocol.

#ifndef ROC_SDP_MEDIA_TRANSPORT_H_
#define ROC_SDP_MEDIA_TRANSPORT_H_

namespace roc {
namespace sdp {

//! Media transport protocol.
enum MediaTransport {
    //! Media transport is not set.
    MediaTransport_None,

    //! RTP/AVP.
    MediaTransport_RTP_AVP
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_MEDIA_TRANSPORT_H_
