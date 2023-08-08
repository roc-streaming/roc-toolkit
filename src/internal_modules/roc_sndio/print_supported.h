/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/print_supported.h
//! @brief Print supported schemes and formats.

#ifndef ROC_SNDIO_PRINT_SUPPORTED_H_
#define ROC_SNDIO_PRINT_SUPPORTED_H_

#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_sndio/backend_dispatcher.h"

namespace roc {
namespace sndio {

//! Print supported schemes and formats.
ROC_ATTR_NODISCARD bool print_supported(BackendDispatcher& backend_dispatcher,
                                        core::IArena& arena);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PRINT_SUPPORTED_H_
