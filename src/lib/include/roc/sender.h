/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/sender.h
 * @brief Roc sender.
 */

#ifndef ROC_SENDER_H_
#define ROC_SENDER_H_

#include "roc/address.h"
#include "roc/config.h"
#include "roc/context.h"
#include "roc/frame.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Roc sender.
 *
 * Sender encodes an audio stream and sends it to a remote receiver.
 *
 * @b Workflow
 *
 * Sender is thread-safe but doesn't have its own thread. The encoding and clocking
 * parts work in the caller context, and might be time-consuming. The network part is
 * delegated to the network thread of the context to which the sender is attached. Sender
 * automatically attaches to a context when created and detaches from it when destroyed.
 *
 * After creating a sender using roc_sender_open(), the user should bind it to a local
 * port using roc_sender_bind(), and then connect to one or multiple remote receiver
 * ports for different types of packets using roc_sender_connect(). After that, the user
 * can iteratively send samples using roc_sender_write(), and eventually destroy the
 * sender using roc_sender_close().
 *
 * @b Ports
 *
 * The user is responsible for connecting the sender to every receiver port required for
 * communication by the selected sender configuration, and selecting the same protocol
 * for every port at the sender and receiver. Currently, two options are possible:
 *
 *  - If the user disabled the FEC using @c ROC_FEC_NONE FEC code, a single RTP stream
 *    is used to transfer the samples. The sender should be connected to a single
 *    @c ROC_PROTO_RTP port.
 *
 *  - Otherwise, a pair of RTP streams is used to transfer the samples, one for the
 *    source (audio) packets, and another for the repair (FEC) packets. Depending on
 *    the selected FEC code, the sender should be connected to a pair of
 *    @c ROC_PROTO_RTP_*_SOURCE and @c ROC_PROTO_*_REPAIR ports.
 *
 * @b Timing
 *
 * Sender should send samples with a constant rate, configured when it is created.
 * There are two ways to accomplish this:
 *
 *  - If the user did specify @c ROC_FLAG_ENABLE_TIMER flag, sender enables automatic
 *    clocking using a CPU timer. roc_sender_write() will block until it's time to
 *    send the next bunch of samples. This mode is useful when the user gets samples
 *    from a non-realtime source, e.g. from an audio file.
 *
 *  - Otherwise, the user should implement clocking by themselves. roc_sender_write()
 *    will send samples immediately, and the user is responsible to call it only
 *    when it's time to send more samples. This mode is useful when the user gets
 *    samples from a realtime source with its own clock, e.g. from an audio device.
 *    Automatic clocking should not be used in this case because the audio device
 *    and sender clocks might have slightly different rates, which will eventually
 *    lead to an underrun or an overrun.
 *
 * @b Thread-safety
 *  - can be used concurrently
 */
typedef struct roc_sender roc_sender;

/** Open a new sender.
 *
 * Allocate and initialize a new sender, and attach it to the context.
 *
 * @b Parameters
 *  - @p context should point to an opened context. The context should not be closed
 *    until the sender is destroyed.
 *  - @p config defines sender parameters. If @p config is NULL, default values are used
 *    for all parameters. Otherwise, default values are used for parameters set to zero.
 *
 * @b Returns
 *  - returns a new sender if it was successfully created
 *  - returns NULL if the arguments are invalid
 *  - returns NULL if there is not enough memory
 */
ROC_API roc_sender* roc_sender_open(roc_context* context,
                                    const roc_sender_config* config);

/** Bind sender to a local port.
 *
 * Binds the sender to a local port. Should be called exactly once before calling
 * roc_sender_write() first time. If @p address port is zero, the sender is bound
 * to a randomly chosen ephemeral port. If the function succeeds, the actual port
 * to which the sender was bound is written back to @p address.
 *
 * It doesn't matter whether the context thread is already started or not.
 *
 * @b Parameters
 *  - @p sender should point to an opened sender
 *  - @p address should point to a properly initialized address
 *
 * @b Returns
 *  - returns zero if the sender was successfully bound to a port
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the sender is already bound
 *  - returns a negative value if a network error occurred
 */
ROC_API int roc_sender_bind(roc_sender* sender, roc_address* address);

/** Connect sender to a remote receiver port.
 *
 * Connects the sender to a receiver port. Should be called one or multiple times
 * before calling roc_sender_write() first time.
 *
 * It doesn't matter whether the context thread is already started or not.
 *
 * @b Parameters
 *  - @p sender should point to an opened sender
 *  - @p proto should specify the protocol expected by receiver on the port
 *  - @p address should specify the address of the receiver port
 *
 * @b Returns
 *  - returns zero if the sender was successfully connected to a port
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if roc_sender_write() was already called first time
 */
ROC_API int roc_sender_connect(roc_sender* sender,
                               roc_port_type type,
                               roc_protocol proto,
                               const roc_address* address);

/** Encode and send samples.
 *
 * Encodes samples to packets and enqueues them to be sent by the context network
 * thread. Should be called after roc_sender_bind() and roc_sender_connect(). If
 * automatic clocking is enabled, blocks until it's time to send more samples.
 * The function returns after encoding and enqueuing the packets, without waiting
 * the packets to be actually sent.
 *
 * If the context thread is not started yet, packets will not be actually sent until
 * it's started, which may cause receiver to consider them outdated and drop them
 * when they are finally delivered.
 *
 * @b Parameters
 *  - @p sender should point to an opened, bound, and connected sender
 *  - @p frame should point to a valid frame with an array of samples to send
 *
 * @b Returns
 *  - returns zero if all samples were successfully encoded and enqueued
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the sender is not bound or connected
 *  - returns a negative value if there is not enough memory
 */
ROC_API int roc_sender_write(roc_sender* sender, const roc_frame* frame);

/** Close sender.
 *
 * Deinitialize and deallocate sender, and detach it from the context. The user should
 * ensure that nobody uses the sender since this function is called. If this function
 * fails, the sender is kept opened and attached to the context.
 *
 * @b Parameters
 *  - @p sender should point to an opened sender
 *
 * @b Returns
 *  - returns zero if the sender was successfully closed
 *  - returns a negative value if the arguments are invalid
 */
ROC_API int roc_sender_close(roc_sender* sender);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_SENDER_H_ */
