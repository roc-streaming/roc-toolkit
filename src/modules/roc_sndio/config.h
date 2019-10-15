/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/config.h
//! @brief Sink and source config.

#ifndef ROC_SNDIO_CONFIG_H_
#define ROC_SNDIO_CONFIG_H_

#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace sndio {

//! Sink and source config.
struct Config {
    //! Channel mask.
    packet::channel_mask_t channels;

    //! Number of samples per channel per seconds.
    size_t sample_rate;

    //! Duration of the internal frames, in nanoseconds.
    core::nanoseconds_t frame_length;

    //! Requested input or output latency.
    core::nanoseconds_t latency;

    //! Initialize.
    Config()
        : channels(0)
        , sample_rate(0)
        , frame_length(0)
        , latency(0) {
    }
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_CONFIG_H_
