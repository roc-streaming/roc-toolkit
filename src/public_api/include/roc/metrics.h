/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/metrics.h
 * \brief Metrics.
 */

#ifndef ROC_METRICS_H_
#define ROC_METRICS_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Receiver session metrics.
 *
 * Represents metrics of single session connected to receiver.
 */
typedef struct roc_session_metrics {
    /** Estimated network-incoming-queue latency, in nanoseconds.
     * Defines how much media is buffered in receiver packet queue.
     */
    unsigned long long niq_latency;

    /** Estimated end-to-end latency, in nanoseconds.
     *
     * Defines how much time passes after frame is written to sender
     * and before it is read from receiver.
     *
     * Computations are based on RTCP and NTP. If \ref ROC_PROTO_RTCP is not used,
     * latency will be zero. If NTP clocks of sender and receiver are not synchronized,
     * latency will be calculated incorrectly.
     *
     * May be zero initially until enough data is transferred.
     */
    unsigned long long e2e_latency;
} roc_session_metrics;

/** Receiver slot metrics.
 *
 * Represents metrics of single receiver slot.
 */
typedef struct roc_receiver_metrics {
    /** Number of sessions connected to receiver slot.
     */
    unsigned int num_sessions;

    /** Pointer to user-defined buffer for session metrics.
     * If user sets this pointer, it is used to write metrics for
     * individual sessions.
     */
    roc_session_metrics* sessions;

    /** Number of structs in session metrics buffer.
     * If \c sessions pointer is set, \c sessions_size should define its
     * size. If number of sessions is greater than provided size, only
     * metrics for first \c sessions_size sessions will be written. Total
     * number of sessions is always written to \c num_sessions.
     */
    size_t sessions_size;
} roc_receiver_metrics;

/** Sender slot metrics.
 *
 * Represents metrics of single sender slot.
 */
typedef struct roc_sender_metrics {
    /** Do not use.
     */
    int unused;
} roc_sender_metrics;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_METRICS_H_ */
