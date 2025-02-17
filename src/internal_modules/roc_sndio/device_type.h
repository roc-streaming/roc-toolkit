/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/device_type.h
//! @brief Device type.

#ifndef ROC_SNDIO_DEVICE_TYPE_H_
#define ROC_SNDIO_DEVICE_TYPE_H_

namespace roc {
namespace sndio {

//! Device type.
enum DeviceType {
    DeviceType_Sink,  //!< Sink.
    DeviceType_Source //!< Source.
};

//! Convert device type to string.
const char* device_type_to_str(DeviceType type);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_DEVICE_TYPE_H_
