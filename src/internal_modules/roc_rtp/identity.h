/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/identity.h
//! @brief RTP participant identity.

#ifndef ROC_RTP_IDENTITY_H_
#define ROC_RTP_IDENTITY_H_

#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/uuid.h"
#include "roc_packet/units.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtp {

//! RTP participant identity.
class Identity : public core::NonCopyable<> {
public:
    //! Initialize.
    Identity();

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get generated CNAME.
    //! Uniquely identifies participant across all RTP sessions.
    //! It is expected that collisions are not practically possible.
    const char* cname() const;

    //! Get generated SSRC.
    //! Uniquely identifies participant within RTP session.
    //! It is expected that collisions are possible and should be resolved.
    packet::stream_source_t ssrc() const;

    //! Regenerate SSRC.
    //! Used in case of SSRC collision.
    ROC_ATTR_NODISCARD status::StatusCode change_ssrc();

private:
    char cname_[core::UuidLen + 1];
    packet::stream_source_t ssrc_;

    status::StatusCode init_status_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_IDENTITY_H_
