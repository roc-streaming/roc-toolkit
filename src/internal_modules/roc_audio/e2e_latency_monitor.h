/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/e2e_latency_monitor.h
//! @brief End-to-end latency monitor.

#ifndef ROC_AUDIO_E2E_LATENCY_MONITOR_H_
#define ROC_AUDIO_E2E_LATENCY_MONITOR_H_

#include "roc_audio/frame.h"
#include "roc_audio/iframe_reader.h"
#include "stddef.h"

namespace roc {
namespace audio {

//! Keeps track of current overall latency for a specific participant per stream.
class EndToEndLatencyMonitor : public IFrameReader, public core::NonCopyable<> {
public:
    //! Constructor.
    EndToEndLatencyMonitor(IFrameReader& reader);

    //! Destructor.
    virtual ~EndToEndLatencyMonitor();

    //! Read audio frame from a pipeline.
    virtual bool read(Frame& frame);

    //! Is latency already available.
    //! @returns
    //!  true if the last frame contained non-zero capture timestamp.
    bool has_latency() const;

    //! Get last valid latency value.
    core::nanoseconds_t latency() const;

private:
    IFrameReader& reader_;

    bool ready_;
    core::nanoseconds_t e2e_latency_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_E2E_LATENCY_MONITOR_H_
