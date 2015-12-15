/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_datagram/default_buffer_composer.h
//! @brief Datagram composer interface.

#ifndef ROC_DATAGRAM_DEFAULT_BUFFER_COMPOSER_H_
#define ROC_DATAGRAM_DEFAULT_BUFFER_COMPOSER_H_

#include "roc_config/config.h"
#include "roc_core/byte_buffer.h"

namespace roc {
namespace datagram {

//! Default composer for datagram's buffer.
static inline core::IByteBufferComposer& default_buffer_composer() {
    return core::ByteBufferTraits::default_composer<ROC_CONFIG_MAX_UDP_BUFSZ>();
}

} // namespace datagram
} // namespace roc

#endif // ROC_DATAGRAM_DEFAULT_BUFFER_COMPOSER_H_
