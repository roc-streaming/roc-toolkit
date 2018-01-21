/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/frame.h
 * @brief Audio frame.
 */

#ifndef ROC_FRAME_H_
#define ROC_FRAME_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Audio frame.
 *
 * Represents a multichannel sequence of audio samples. The user is responsible for
 * allocating and deallocating the frame and its sample buffer.
 *
 * @b Thread-safety
 *  - should not be used concurrently
 */
typedef struct roc_frame {
    /** Audio samples.
     *
     * Points to an array of num_samples native endian floating point PCM samples. Sample
     * values are in range [0; 1]. Sample rate and channel set are defined by the sender
     * or receiver configuration. Multiple channels are interleaved, e.g. two channels
     * are encoded as "L R L R ...".
     */
    float* samples;

    /** Number of samples.
     *
     * Defines the number of samples in the sample buffer for all channels. Should be a
     * multiple of the number of channels.
     */
    size_t num_samples;
} roc_frame;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_FRAME_H_ */
