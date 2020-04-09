/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/media_proto.h
//! @brief SDP media description transport protocol.

#ifndef ROC_SDP_MEDIA_PROTO_H_
#define ROC_SDP_MEDIA_PROTO_H_

namespace roc {
namespace sdp {

//! Address family.
enum MediaProto {
    //! Media proto is not set.
    MediaProto_None,

    //! RTP/AVP.
    MediaProto_RTP_AVP
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_MEDIA_PROTO_H_
