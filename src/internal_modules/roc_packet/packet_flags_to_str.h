/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet_flags_to_str.h
//! @brief Packet flags to string.

#ifndef ROC_PACKET_PACKET_FLAGS_TO_STR_H_
#define ROC_PACKET_PACKET_FLAGS_TO_STR_H_

#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Format packet flags to string.
class packet_flags_to_str : public core::NonCopyable<> {
public:
    //! Construct.
    explicit packet_flags_to_str(unsigned flags);

    //! Get formatted string.
    const char* c_str() const {
        return buf_;
    }

private:
    char buf_[256];
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_FLAGS_TO_STR_H_
