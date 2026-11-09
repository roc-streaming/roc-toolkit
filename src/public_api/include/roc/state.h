/*
 * Copyright (c) 2026 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/state.h
 * \brief Sender and receiver state.
 */

#ifndef ROC_STATE_H_
#define ROC_STATE_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Sender or receiver state.
 *
 * A set of states can be passed to roc_receiver_poll() and roc_sender_poll()
 * as a bitmask, e.g. <tt>ROC_STATE_ACTIVE | ROC_STATE_BROKEN</tt>.
 * At any moment, sender or receiver is exactly in one of these states.
 */
typedef enum roc_state {
    /** Audio is being streamed.
     *
     * On receiver, it means that there is at least one connected sender.
     * On sender, it means that at least one slot is connected.
     */
    ROC_STATE_ACTIVE = (1 << 0),

    /** Idle state: ready, but not actively streaming audio.
     *
     * Receiver in this state produces silence, and sender in this state does
     * not produce packets. It is safe to pause the associated audio device.
     */
    ROC_STATE_IDLE = (1 << 1),

    /** Paused state
     *
     * Reserved for the upcoming pause and resume operations. It is never
     * reported by the current implementation.
     */
    ROC_STATE_PAUSED = (1 << 2),

    /** Broken and can't be used anymore.
     *
     * The only allowed operation is to close sender or receiver.
     */
    ROC_STATE_BROKEN = (1 << 3),

    /** Closed and can't be used anymore.
     */
    ROC_STATE_CLOSED = (1 << 4)
} roc_state;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_STATE_H_ */
