/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_ENDTOENDLATENCYMONITOR_H_
#define ROC_ENDTOENDLATENCYMONITOR_H_

#include "stddef.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/frame.h"

namespace roc {
namespace audio {

//! Keeps track of current overall latency for a specific participant per stream.
class EndToEndLatencyMonitor : public IFrameReader,  public core::NonCopyable<>  {
public:

    //! Constructor.
    EndToEndLatencyMonitor(IFrameReader& reader);
    //! Destructor.
    virtual ~EndToEndLatencyMonitor();

    //! Is e2e_latency info valid.
    //! @returns
    //! True if the last frame contained non-zero capture timestamp.
    bool is_valid() const;

    //! Read audio frame from a pipeline.
    virtual bool read(Frame& frame);

    //! Get last valid latency value.
    core::nanoseconds_t latency() const;

private:
    IFrameReader& reader_;

    bool valid_;
    core::nanoseconds_t e2e_latency_;
};

} // namespace audio
} // namespace roc

#endif // ROC_ENDTOENDLATENCYMONITOR_H_
