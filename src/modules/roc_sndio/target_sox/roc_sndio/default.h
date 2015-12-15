/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/default.h
//! @brief Get default driver and device.

#ifndef ROC_SNDIO_DEFAULT_H_
#define ROC_SNDIO_DEFAULT_H_

namespace roc {
namespace sndio {

//! Detect defaults for name and type.
bool detect_defaults(const char** name, const char** type);

//! Get default driver.
const char* default_driver();

//! Get default device.
const char* default_device();

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_DEFAULT_H_
