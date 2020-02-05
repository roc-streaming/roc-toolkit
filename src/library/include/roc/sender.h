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
 * Sender gets an audio stream from the user, encodes it into network packets, and
 * transmits them to a remote receiver.
 *
 * @b Context
 *
 * Sender is automatically attached to a context when opened and detached from it when
 * closed. The user should not close the context until the sender is not closed.
 *
 * Sender work consists of two parts: stream encoding and packet transmission. The
 * encoding part is performed in the sender itself, and the transmission part is
 * performed in the context network worker thread(s).
 *
 * @b Lifecycle
 *
 * A sender is created using roc_sender_open(). Then it should be bound to a local port
 * using roc_sender_bind() and connected to a single or multiple remote receiver ports
 * using roc_sender_connect(). After that, the audio stream is iteratively written to the
 * sender using roc_sender_write(). When the sender is not needed anymore, it is
 * destroyed using roc_sender_close().
 *
 * @b Ports
 *
 * The user is responsible for connecting the sender to all necessary receiver ports
 * and selecting the same port types and protocols as at the receiver side.
 *
 * Currently, two configurations are possible:
 *
 *  - If FEC is disabled, a single port of type @c ROC_PORT_AUDIO_SOURCE should be
 *    connected. The only supported protocol in this case is @c ROC_PROTO_RTP. This port
 *    will be used to send audio packets.
 *
 *  - If FEC is enabled, two ports of types @c ROC_PORT_AUDIO_SOURCE and
 *    @c ROC_PORT_AUDIO_REPAIR should be connected. These ports will be used to send
 *    audio packets and redundant data for audio packets, respectively. The supported
 *    protocols in this case depend on the selected FEC code. For example, if
 *    @c ROC_FEC_RS8M is used, the corresponding protocols would be
 *    @c ROC_PROTO_RTP_RSM8_SOURCE and @c ROC_PROTO_RSM8_REPAIR.
 *
 * @b Resampling
 *
 * If the sample rate of the user frames and the sample rate of the network packets are
 * different, the sender employs resampler to convert one rate to another.
 *
 * Resampling is a quite time-consuming operation. The user can choose between completely
 * disabling resampling (and so use the same rate for frames and packets) or several
 * resampler profiles providing different compromises between CPU consumption and quality.
 *
 * @b Timing
 *
 * Sender should encode samples at a constant rate that is configured when the sender
 * is created. There are two ways to accomplish this:
 *
 *  - If the user enabled internal clock (@c ROC_CLOCK_INTERNAL), the sender employs a CPU
 *    timer to block writes until it's time to encode the next bunch of samples according
 *    to the configured sample rate. This mode is useful when the user gets samples from a
 *    non-realtime source, e.g. from an audio file.
 *
 *  - Otherwise (@c ROC_CLOCK_EXTERNAL), the samples written to the sender are encoded
 *    immediately and the user is responsible to write samples in time. This mode is
 *    useful when the user gets samples from a realtime source with its own clock, e.g.
 *    from an audio device. Internal clock should not be used in this case because the
 *    audio device and the CPU might have slightly different clocks, and the difference
 *    will eventually lead to an underrun or an overrun.
 *
 * @b Thread-safety
 *  - can be used concurrently
 */
typedef struct roc_sender roc_sender;

/** Open a new sender.
 *
 * Allocates and initializes a new sender, and attaches it to the context.
 *
 * @b Parameters
 *  - @p context should point to an opened context
 *  - @p config should point to an initialized config
 *  - @p result should point to an unitialized roc_sender pointer
 *
 * @b Returns
 *  - returns zero if the sender was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if there are not enough resources
 */
ROC_API int roc_sender_open(roc_context* context,
                            const roc_sender_config* config,
                            roc_sender** result);

/** Connect the sender to a remote receiver port.
 *
 * Connects the sender to a receiver port. Should be called one or multiple times
 * before calling roc_sender_write() first time. The @p type and @p proto should be
 * the same as they are set at the receiver for this port.
 *
 * @b Parameters
 *  - @p sender should point to an opened sender
 *  - @p type specifies the receiver port type
 *  - @p proto specifies the receiver port protocol
 *  - @p address should point to a properly initialized address
 *
 * @b Returns
 *  - returns zero if the sender was successfully connected to a port
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if roc_sender_write() was already called
 */
ROC_API int roc_sender_connect(roc_sender* sender,
                               roc_port_type type,
                               roc_protocol proto,
                               const roc_address* address);

/** Encode samples to packets and transmit them to the receiver.
 *
 * Encodes samples to packets and enqueues them for transmission by the context network
 * worker thread. Should be called after roc_sender_bind() and roc_sender_connect().
 *
 * If the automatic timing is enabled, the function blocks until it's time to encode the
 * samples according to the configured sample rate. The function returns after encoding
 * and enqueuing the packets, without waiting when the packets are actually transmitted.
 *
 * @b Parameters
 *  - @p sender should point to an opened, bound, and connected sender
 *  - @p frame should point to a valid frame with an array of samples to send
 *
 * @b Returns
 *  - returns zero if all samples were successfully encoded and enqueued
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the sender is not bound or connected
 *  - returns a negative value if there are not enough resources
 */
ROC_API int roc_sender_write(roc_sender* sender, const roc_frame* frame);

/** Close the sender.
 *
 * Deinitializes and deallocates the sender, and detaches it from the context. The user
 * should ensure that nobody uses the sender during and after this call. If this
 * function fails, the sender is kept opened and attached to the context.
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
