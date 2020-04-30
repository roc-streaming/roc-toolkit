/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/profiler.h
//! @brief Profiler.

#ifndef ROC_AUDIO_PROFILER_H_
#define ROC_AUDIO_PROFILER_H_

#include "roc_audio/frame.h"
#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/refcnt.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Profiler
class Profiler : public core::NonCopyable<> {
public:
    //! Initialization.
    Profiler(core::IAllocator& allocator,
             packet::channel_mask_t channels,
             size_t sample_rate,
             core::nanoseconds_t interval);

    //! Init audio frame.
    void begin_frame(size_t frame_size);

    //! Profile frame speed
    void end_frame(size_t frame_size, core::nanoseconds_t elapsed);

    //! For Testing Only
    double get_moving_avg() {
        return moving_avg_;
    }

private:
    core::RateLimiter rate_limiter_;

    core::nanoseconds_t interval_;

    const size_t chunk_length_;
    const size_t num_chunks_;
    core::Array<double> chunks_;

    double moving_avg_;

    const size_t sample_rate_;
    const size_t num_channels_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROFILER_H_
