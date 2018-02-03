/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox.h
//! @brief Get default driver and device.

#ifndef ROC_SNDIO_SOX_H_
#define ROC_SNDIO_SOX_H_

namespace roc {
namespace sndio {

//! Initialize SoX.
void sox_setup();

//! Detect defaults for name and type.
bool sox_defaults(const char** name, const char** type);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_H_
