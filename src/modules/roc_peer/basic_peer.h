/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_peer/basic_peer.h
//! @brief Base class for peers.

#ifndef ROC_PEER_BASIC_PEER_H_
#define ROC_PEER_BASIC_PEER_H_

#include "roc_core/noncopyable.h"
#include "roc_peer/context.h"

namespace roc {
namespace peer {

//! Base class for peers.
class BasicPeer : public core::NonCopyable<> {
public:
    //! Initialize.
    BasicPeer(Context& context);

    //! Deinitialize.
    virtual ~BasicPeer();

    //! Deinitialize and deallocate peer.
    void destroy();

protected:
    //! Peer's context.
    Context& context_;
};

} // namespace peer
} // namespace roc

#endif // ROC_PEER_BASIC_PEER_H_
