/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_uv/roc_netio/udp_composer.h
//! @brief UDP datagram composer.

#ifndef ROC_NETIO_UDP_COMPOSER_H_
#define ROC_NETIO_UDP_COMPOSER_H_

#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"

#include "roc_datagram/idatagram_composer.h"

#include "roc_netio/udp_datagram.h"

namespace roc {
namespace netio {

//! UDP datagram composer.
class UDPComposer : public datagram::IDatagramComposer, public core::NonCopyable<> {
public:
    virtual ~UDPComposer();

    //! Initialize.
    UDPComposer(core::IPool<UDPDatagram>& pool)
        : pool_(pool) {
    }

    //! Create datagram.
    virtual datagram::IDatagramPtr compose() {
        return new (pool_) UDPDatagram(pool_);
    }

private:
    core::IPool<UDPDatagram>& pool_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_COMPOSER_H_
