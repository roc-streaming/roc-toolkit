/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/print_supported.h
//! @brief Print supported schemes and formats.

#ifndef ROC_ADDRESS_PRINT_SUPPORTED_H_
#define ROC_ADDRESS_PRINT_SUPPORTED_H_

#include "roc_address/protocol_map.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"

namespace roc {
namespace address {

//! Print supported schemes and formats.
ROC_ATTR_NODISCARD bool print_supported(ProtocolMap&, core::IArena&);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_PRINT_SUPPORTED_H_
