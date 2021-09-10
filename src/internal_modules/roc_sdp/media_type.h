/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/media_type.h
//! @brief SDP media description type.

#ifndef ROC_SDP_MEDIA_TYPE_H_
#define ROC_SDP_MEDIA_TYPE_H_

namespace roc {
namespace sdp {

//! Media type.
enum MediaType {
    //! Media type is not set.
    MediaType_None,

    //! audio.
    MediaType_Audio,

    //! video.
    MediaType_Video,

    //! text.
    MediaType_Text,

    //! application.
    MediaType_Application
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_MEDIA_TYPE_H_
