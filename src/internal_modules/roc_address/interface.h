/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/interface.h
//! @brief Interface ID.

#ifndef ROC_ADDRESS_INTERFACE_H_
#define ROC_ADDRESS_INTERFACE_H_

namespace roc {
namespace address {

//! Interface ID.
enum Interface {
    //! Invalid interface.
    Iface_Invalid,

    //! Interface that aggregates multiple types of streams (e.g. RTSP).
    Iface_Aggregate,

    //! Source packets of audio stream (e.g. RTP or RTP + RS8M).
    Iface_AudioSource,

    //! Repair packets of audio stream (e.g. RS8M).
    Iface_AudioRepair,

    //! Control packets of audio stream (e.g. RTCP).
    Iface_AudioControl,

    //! Number of interfaces.
    Iface_Max
};

//! Get string name of the interface.
const char* interface_to_str(Interface);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_INTERFACE_H_
