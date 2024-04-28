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

/** Metrics for a single connection between sender and receiver.
 *
 * On receiver, represents one connected sender. Similarly, on sender
 * represents one connected receiver. It doesn't matter who initiated
 * connection, sender or receiver.
 *
 * Some metrics are calculated locally, and some are periodically retrieved
 * from remote side via control protocol like \ref ROC_PROTO_RTCP.
 */
typedef struct roc_connection_metrics {
    /** Estimated end-to-end latency, in nanoseconds.
     *
     * Defines how much time passes after a frame is written to sender and before
     * it is read from receiver. Consists of sender latency, network latency,
     * and receiver latency.
     *
     * Computations are based on RTCP and system clock. If \ref ROC_PROTO_RTCP is
     * not used, latency will be zero. If system clocks of sender and receiver are
     * not synchronized, latency will be calculated incorrectly.
     *
     * May be zero initially, until enough statistics is accumulated.
     */
    unsigned long long e2e_latency;

    /** Estimated interarrival jitter, in nanoseconds.
     *
     * Determines expected variance of inter-packet arrival period.
     *
     * Estimated on receiver.
     */
    unsigned long long mean_jitter;

    /** Total amount of packets that receiver expects to be delivered.
     */
    unsigned long long expected_packets;

    /** Cumulative count of lost packets.
     *
     *  The total number of RTP data packets that have been lost since the beginning
     *  of reception.
     */
    unsigned long long lost_packets;
} roc_connection_metrics;

/** Receiver metrics.
 *
 * Holds receiver-side metrics that are not specific to connection.
 * If multiple slots are used, each slot has its own metrics.
 */
typedef struct roc_receiver_metrics {
    /** Number of active connections.
     *
     * Defines how much senders are currently connected to receiver.
     * When there are no connections, receiver produces silence.
     */
    unsigned int connection_count;
} roc_receiver_metrics;

/** Sender metrics.
 *
 * Holds sender-side metrics that are not specific to connection.
 * If multiple slots are used, each slot has its own metrics.
 */
typedef struct roc_sender_metrics {
    /** Number of active connections.
     *
     * Defines how much receivers are currently discovered.
     *
     * If a control or signaling protocol like \ref ROC_PROTO_RTSP or
     * \ref ROC_PROTO_RTCP is not used, sender doesn't know about receivers and
     * doesn't have connection metrics.
     *
     * If such a protocol is used, in case of unicast, sender will have a single
     * connection, and in case of multicast, sender may have multiple
     * connections, one per each discovered receiver.
     */
    unsigned int connection_count;
} roc_sender_metrics;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_METRICS_H_ */
