/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/media_description.h
//! @brief Media Description Protocol

#ifndef ROC_SDP_MEDIA_DESCRIPTION_H_
#define ROC_SDP_MEDIA_DESCRIPTION_H_

#include "roc_address/socket_addr.h"
#include "roc_core/log.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"

namespace roc {
namespace sdp {

//! SDP media description.
class MediaDescription : public core::RefCnt<MediaDescription>, 
                         public core::ListNode {
public:
    //! Clear all fields.
    void clear();

    //! Initialize empty media description
    MediaDescription(core::IAllocator& allocator);

    //! Media field.
    // m=<type> <port> <proto> <fmt>.
    const char* media() const;

    //! Set media field.
    //! String should not be zero-terminated.
    bool set_media(const char* str, size_t str_len);

private:
    friend class core::RefCnt<MediaDescription>;

    void destroy();

    core::StringBuffer<> media_field_;
    address::SocketAddr connection_address_;
    
    core::IAllocator& allocator_;
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_MEDIA_DESCRIPTION_H_
