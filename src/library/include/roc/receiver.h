/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/receiver.h
 * @brief Roc receiver.
 */

#ifndef ROC_RECEIVER_H_
#define ROC_RECEIVER_H_

#include "roc/address.h"
#include "roc/config.h"
#include "roc/context.h"
#include "roc/frame.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Roc receiver.
 *
 * Receiver receives the network packets from multiple senders, decodes audio streams
 * from them, mixes multiple streams into a single stream, and returns it to the user.
 *
 * @b Context
 *
 * Receiver is automatically attached to a context when opened and detached from it when
 * closed. The user should not close the context until the receiver is not closed.
 *
 * Receiver work consists of two parts: packet reception and stream decoding. The
 * decoding part is performed in the receiver itself, and the reception part is
 * performed in the context network worker thread(s).
 *
 * @b Lifecycle
 *
 * A receiver is created using roc_receiver_open(). Then it should be bound to a single
 * or multiple local ports using roc_receiver_bind(). After that, the audio stream is
 * iteratively read from the receiver using roc_receiver_read(). When the receiver is not
 * needed anymore, it is destroyed using roc_receiver_close().
 *
 * @b Ports
 *
 * Receiver can be bound to multiple network ports of several types. Every port handles
 * packets of the specific protocol selected when the port is bound. It is allowed to
 * bind multiple ports of the same type, typically handling different protocols.
 *
 * Senders can then be connected to some or all receiver ports to transmit one or several
 * packet streams. If a sender employs FEC, it needs to be connected to a pair of
 * @c ROC_PORT_AUDIO_SOURCE and @c ROC_PORT_AUDIO_REPAIR ports which protocols correspond
 * to the employed FEC code. Otherwise, the sender needs to be connected to a single
 * @c ROC_PORT_AUDIO_SOURCE port.
 *
 * @b Sessions
 *
 * Receiver creates a session object for every sender connected to it. Sessions can appear
 * and disappear at any time. Multiple sessions can be active at the same time.
 *
 * A session is identified by the sender address. A session may contain multiple packet
 * streams sent to different receiver ports. If the sender employs FEC, the session will
 * contain source and repair packet streams. Otherwise, the session will contain a single
 * source packet stream.
 *
 * A session is created automatically on the reception of the first packet from a new
 * address and destroyed when there are no packets during a timeout. A session is also
 * destroyed on other events like a large latency underrun or overrun or broken playback,
 * but if the sender continues to send packets, it will be created again shortly.
 *
 * @b Mixing
 *
 * Receiver mixes audio streams from all currently active sessions into a single output
 * stream. The output stream continues no matter how much active sessions there are at
 * the moment. In particular, if there are no sessions, the receiver produces a stream
 * with all zeros. Sessions can be added and removed from the output stream at any time,
 * probably in the middle of a frame.
 *
 * @b Resampling
 *
 * Every session may have a different sample rate. And even if nominally all of them are
 * of the same rate, device frequencies usually differ by a few tens of Hertz.
 *
 * Receiver compensates these differences by adjusting the rate of every session stream to
 * the rate of the receiver output stream using a per-session resampler. The frequencies
 * factor between the sender and the receiver clocks is calculated dynamically for every
 * session based on the session incoming packet queue size.
 *
 * Resampling is a quite time-consuming operation. The user can choose between completely
 * disabling resampling (at the cost of occasional underruns or overruns) or several
 * resampler profiles providing different compromises between CPU consumption and quality.
 *
 * @b Timing
 *
 * Receiver should decode samples at a constant rate that is configured when the receiver
 * is created. There are two ways to accomplish this:
 *
 *  - If the user enabled internal clock (@c ROC_CLOCK_INTERNAL), the receiver employs a
 *    CPU timer to block reads until it's time to decode the next bunch of samples
 *    according to the configured sample rate. This mode is useful when the user passes
 *    samples to a non-realtime destination, e.g. to an audio file.
 *
 *  - Otherwise (@c ROC_CLOCK_EXTERNAL), the samples read from the receiver are decoded
 *    immediately and the user is responsible to read samples in time. This mode is
 *    useful when the user passes samples to a realtime destination with its own clock,
 *    e.g. to an audio device. Internal clock should not be used in this case because the
 *    audio device and the CPU might have slightly different clocks, and the difference
 *    will eventually lead to an underrun or an overrun.
 *
 * @b Thread-safety
 *  - can be used concurrently
 */
typedef struct roc_receiver roc_receiver;

/** Open a new receiver.
 *
 * Allocates and initializes a new receiver, and attaches it to the context.
 *
 * @b Parameters
 *  - @p context should point to an opened context
 *  - @p config should point to an initialized config
 *  - @p result should point to an unitialized roc_receiver pointer
 *
 * @b Returns
 *  - returns zero if the receiver was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if there are not enough resources
 */
ROC_API int roc_receiver_open(roc_context* context,
                              const roc_receiver_config* config,
                              roc_receiver** result);

/** Bind the receiver to a local port.
 *
 * Binds the receiver to a local port. May be called multiple times to bind multiple
 * port. May be called at any time.
 *
 * If @p address has zero port, the receiver is bound to a randomly chosen ephemeral
 * port. If the function succeeds, the actual port to which the receiver was bound
 * is written back to @p address.
 *
 * @b Parameters
 *  - @p receiver should point to an opened receiver
 *  - @p type specifies the port type
 *  - @p proto specifies the port protocol
 *  - @p address should point to a properly initialized address
 *
 * @b Returns
 *  - returns zero if the receiver was successfully bound to a port
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the address can't be bound
 *  - returns a negative value if there are not enough resources
 */
ROC_API int roc_receiver_bind(roc_receiver* receiver,
                              roc_port_type type,
                              roc_protocol proto,
                              roc_address* address);

/** Read samples from the receiver.
 *
 * Reads network packets received on bound ports, routes packets to sessions, repairs lost
 * packets, decodes samples, resamples and mixes them, and finally stores samples into the
 * provided frame.
 *
 * If the automatic timing is enabled, the function blocks until it's time to decode the
 * samples according to the configured sample rate.
 *
 * @b Parameters
 *  - @p receiver should point to an opened receiver
 *  - @p frame should point to an initialized frame which will be filled with samples;
 *    the number of samples is defined by the frame size
 *
 * @b Returns
 *  - returns zero if all samples were successfully decoded
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if there are not enough resources
 */
ROC_API int roc_receiver_read(roc_receiver* receiver, roc_frame* frame);

/** Close the receiver.
 *
 * Deinitializes and deallocates the receiver, and detaches it from the context. The user
 * should ensure that nobody uses the receiver during and after this call. If this
 * function fails, the receiver is kept opened and attached to the context.
 *
 * @b Parameters
 *  - @p receiver should point to an opened receiver
 *
 * @b Returns
 *  - returns zero if the receiver was successfully closed
 *  - returns a negative value if the arguments are invalid
 */
ROC_API int roc_receiver_close(roc_receiver* receiver);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_RECEIVER_H_ */
