/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample_buffer.h
//! @brief Sample buffer.

#ifndef ROC_AUDIO_SAMPLE_BUFFER_H_
#define ROC_AUDIO_SAMPLE_BUFFER_H_

#include "roc_config/config.h"
#include "roc_core/buffer_traits.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Sample buffer traits.
typedef core::BufferTraits<packet::sample_t> SampleBufferTraits;

//! Sample buffer interface.
typedef SampleBufferTraits::Interface ISampleBuffer;

//! Sample buffer smart pointer.
typedef SampleBufferTraits::Ptr ISampleBufferPtr;

//! Const sample buffer smart pointer.
typedef SampleBufferTraits::ConstPtr ISampleBufferConstPtr;

//! Sample buffer slice.
typedef SampleBufferTraits::Slice ISampleBufferSlice;

//! Const sample buffer slice.
typedef SampleBufferTraits::ConstSlice ISampleBufferConstSlice;

//! Sample buffer composer interface.
typedef SampleBufferTraits::Composer ISampleBufferComposer;

//! Default composer for sample buffers.
static inline ISampleBufferComposer& default_buffer_composer() {
    return SampleBufferTraits::
        default_composer<ROC_CONFIG_MAX_CHANNELS
                         * ROC_CONFIG_DEFAULT_RECEIVER_TICK_SAMPLES>();
}

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_BUFFER_H_
