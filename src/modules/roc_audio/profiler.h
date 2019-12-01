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
    struct FrameNode : public core::RefCnt<FrameNode>, public core::ListNode {
        FrameNode(size_t s, core::nanoseconds_t t, core::IAllocator& a)
            : samples(s)
            , time(t)
            , allocator_(a) {
        }

        void destroy() {
            allocator_.destroy(*this);
        }

        size_t samples;
        core::nanoseconds_t time;
        core::IAllocator& allocator_;
    };

    core::RateLimiter rate_limiter_;

    core::IAllocator& allocator_;

    core::List<FrameNode> running_data_;
    core::nanoseconds_t interval_;
    core::nanoseconds_t running_samples_time_;
    size_t running_samples_;
    double moving_avg_;

    const size_t sample_rate_;
    const size_t num_channels_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROFILER_H_
