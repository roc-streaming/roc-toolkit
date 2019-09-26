/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_stdio/roc_sndio/print_drivers.h
//! @brief Print drivers.

#ifndef ROC_SNDIO_PRINT_DRIVERS_H_
#define ROC_SNDIO_PRINT_DRIVERS_H_

#include "roc_core/iallocator.h"

namespace roc {
namespace sndio {

//! Print list of supported drivers.
bool print_drivers(core::IAllocator& allocator);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PRINT_DRIVERS_H_
