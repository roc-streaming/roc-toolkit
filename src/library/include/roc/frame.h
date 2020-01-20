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
 * allocating and deallocating the frame and the data it is pointing to.
 *
 * @b Thread-safety
 *  - should not be used concurrently
 */
typedef struct roc_frame {
    /** Audio samples.
     * Sample rate, channel set, and encoding are defined by the sender or
     * receiver parameters.
     */
    void* samples;

    /** Sample buffer size.
     * Defines the size of samples buffer in bytes.
     */
    size_t samples_size;
} roc_frame;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_FRAME_H_ */
