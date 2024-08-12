/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/io_config.h
//! @brief Sink and source config.

#ifndef ROC_SNDIO_IO_CONFIG_H_
#define ROC_SNDIO_IO_CONFIG_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace sndio {

//! Default frame length.
//!  10ms is rather high, but works well even on cheap sound cards and CPUs.
//!  Usually you can use much lower values.
const core::nanoseconds_t DefaultFrameLength = 10 * core::Millisecond;

//! Sink and source config.
struct IoConfig {
    //! Sample spec
    audio::SampleSpec sample_spec;

    //! Duration of the internal frames, in nanoseconds.
    core::nanoseconds_t frame_length;

    //! Requested input or output latency.
    core::nanoseconds_t latency;

    //! Initialize.
    IoConfig()
        : frame_length(DefaultFrameLength)
        , latency(0) {
    }
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IO_CONFIG_H_
