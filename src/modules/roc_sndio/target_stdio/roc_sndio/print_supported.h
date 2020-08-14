/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_stdio/roc_sndio/print_supported.h
//! @brief Print supported schemes and formats.

#ifndef ROC_SNDIO_PRINT_SUPPORTED_H_
#define ROC_SNDIO_PRINT_SUPPORTED_H_

#include "roc_core/iallocator.h"

namespace roc {
namespace sndio {

//! Print supported schemes and formats.
bool print_supported(core::IAllocator& allocator, BackendDispatcher& backend_dispatcher);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PRINT_SUPPORTED_H_
