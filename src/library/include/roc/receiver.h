/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/receiver.h
 * \brief Roc receiver.
 */

#ifndef ROC_RECEIVER_H_
#define ROC_RECEIVER_H_

#include "roc/config.h"
#include "roc/context.h"
#include "roc/endpoint.h"
#include "roc/frame.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Receiver peer.
 *
 * Receiver gets the network packets from multiple senders, decodes audio streams
 * from them, mixes multiple streams into a single stream, and returns it to the user.
 *
 * **Context**
 *
 * Receiver is automatically attached to a context when opened and detached from it when
 * closed. The user should not close the context until the receiver is closed.
 *
 * Receiver work consists of two parts: packet reception and stream decoding. The
 * decoding part is performed in the receiver itself, and the reception part is
 * performed in the context network worker threads.
 *
 * **Life cycle**
 *
 * - A receiver is created using roc_receiver_open().
 *
 * - The receiver either binds local endpoints using roc_receiver_bind(), allowing senders
 *   connecting to them, or itself connects to remote sender endpoints using
 *   roc_receiver_connect(). What option to use is up to the user.
 *
 * - The audio stream is iteratively read from the receiver using roc_receiver_read().
 *   Receiver returns the mixed stream from all connected senders.
 *
 * - The receiver is destroyed using roc_receiver_close().
 *
 * **Interfaces and endpoints**
 *
 * Receiver has several *interfaces*, one per each type defined in \ref roc_interface. The
 * interface defines the type of the comminication with the remote peer and the set of
 * the protocols supported by it.
 *
 * Supported actions with the interface:
 *
 *  - Call roc_receiver_bind() to bind the interface to a local \ref roc_endpoint. In this
 *    case the receiver accepts connections from senders mixes their streams into the
 *    single output stream.
 *
 *  - Call roc_receiver_connect() to connect the interface to a remote \ref roc_endpoint.
 *    In this case the receiver initiates connection to the sender and requests it
 *    to start sending media stream to the receiver.
 *
 * Supported interface configurations:
 *
 *   - Bind \c ROC_INTERFACE_AGGREGATE to a local endpoint (e.g. be an RTSP server).
 *   - Connect \c ROC_INTERFACE_AGGREGATE to a remote endpoint (e.g. be an RTSP client).
 *   - Bind \c ROC_INTERFACE_AUDIO_SOURCE to a local endpoint (e.g. be an RTP receiver).
 *   - Bind \c ROC_INTERFACE_AUDIO_SOURCE and \c ROC_INTERFACE_AUDIO_REPAIR to a pair
 *     of local endpoints (e.g. be an RTP + FECFRAME receiver).
 *
 * **FEC scheme**
 *
 * If \c ROC_INTERFACE_AGGREGATE is used, it automatically creates all necessary transport
 * interfaces and the user should not bother about them.
 *
 * Otherwise, the user should manually configure \c ROC_INTERFACE_AUDIO_SOURCE and
 * \c ROC_INTERFACE_AUDIO_REPAIR interfaces:
 *
 *  - If FEC is disabled (\ref ROC_FEC_DISABLE), only \c ROC_INTERFACE_AUDIO_SOURCE should
 *    be configured. It will be used to transmit audio packets.
 *
 *  - If FEC is enabled, both \c ROC_INTERFACE_AUDIO_SOURCE and
 *    \c ROC_INTERFACE_AUDIO_REPAIR interfaces should be configured. The second interface
 *    will be used to transmit redundant repair data.
 *
 * The protocols for the two interfaces should correspond to each other and to the FEC
 * scheme. For example, if \c ROC_FEC_RS8M is used, the protocols should be
 * \c ROC_PROTO_RTP_RS8M_SOURCE and \c ROC_PROTO_RS8M_REPAIR.
 *
 * **Sessions**
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
 * **Mixing**
 *
 * Receiver mixes audio streams from all currently active sessions into a single output
 * stream.
 *
 * The output stream continues no matter how much active sessions there are at the moment.
 * In particular, if there are no sessions, the receiver produces a stream with all zeros.
 *
 * Sessions can be added and removed from the output stream at any time, probably in the
 * middle of a frame.
 *
 * **Sample rate**
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
 * **Clock source**
 *
 * Receiver should decode samples at a constant rate that is configured when the receiver
 * is created. There are two ways to accomplish this:
 *
 *  - If the user enabled internal clock (\c ROC_CLOCK_INTERNAL), the receiver employs a
 *    CPU timer to block reads until it's time to decode the next bunch of samples
 *    according to the configured sample rate.
 *
 *    This mode is useful when the user passes samples to a non-realtime destination,
 *    e.g. to an audio file.
 *
 *  - If the user enabled external clock (\c ROC_CLOCK_EXTERNAL), the samples read from
 *    the receiver are decoded immediately and hence the user is responsible to call
 *    read operation according to the sample rate.
 *
 *    This mode is useful when the user passes samples to a realtime destination with its
 *    own clock, e.g. to an audio device. Internal clock should not be used in this case
 *    because the audio device and the CPU might have slightly different clocks, and the
 *    difference will eventually lead to an underrun or an overrun.
 *
 * **Thread safety**
 *
 * Can be used concurrently.
 */
typedef struct roc_receiver roc_receiver;

/** Open a new receiver.
 *
 * Allocates and initializes a new receiver, and attaches it to the context.
 *
 * **Parameters**
 *  - \p context should point to an opened context
 *  - \p config should point to an initialized config
 *  - \p result should point to an unitialized roc_receiver pointer
 *
 * **Returns**
 *  - returns zero if the receiver was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 */
ROC_API int roc_receiver_open(roc_context* context,
                              const roc_receiver_config* config,
                              roc_receiver** result);

/** Bind the receiver interface to a local endpoint.
 *
 * Checks that the endpoint is valid and supported by the interface, allocates
 * a new ingoing port, and binds it to the local endpoint.
 *
 * Each interface can be bound or connected only once.
 * May be called multiple times for different interfaces.
 *
 * If \p endpoint has explicitly set zero port, the receiver is bound to a randomly
 * chosen ephemeral port. If the function succeeds, the actual port to which the
 * receiver was bound is written back to \p endpoint.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *  - \p iface specifies the receiver interface
 *  - \p endpoint specifies the receiver endpoint
 *
 * **Returns**
 *  - returns zero if the receiver was successfully bound to a port
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the address can't be bound
 *  - returns a negative value on resource allocation failure
 */
ROC_API int
roc_receiver_bind(roc_receiver* receiver, roc_interface iface, roc_endpoint* endpoint);

/** Read samples from the receiver.
 *
 * Reads network packets received on bound ports, routes packets to sessions, repairs lost
 * packets, decodes samples, resamples and mixes them, and finally stores samples into the
 * provided frame.
 *
 * If \c ROC_CLOCK_INTERNAL is used, the function blocks until it's time to decode the
 * samples according to the configured sample rate.
 *
 * Until the receiver is connected to at least one sender, it produces silence.
 * If the receiver is connected to multiple senders, it mixes their streams into one.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *  - \p frame should point to an initialized frame which will be filled with samples;
 *    the number of samples is defined by the frame size
 *
 * **Returns**
 *  - returns zero if all samples were successfully decoded
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 */
ROC_API int roc_receiver_read(roc_receiver* receiver, roc_frame* frame);

/** Close the receiver.
 *
 * Deinitializes and deallocates the receiver, and detaches it from the context. The user
 * should ensure that nobody uses the receiver during and after this call. If this
 * function fails, the receiver is kept opened and attached to the context.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *
 * **Returns**
 *  - returns zero if the receiver was successfully closed
 *  - returns a negative value if the arguments are invalid
 */
ROC_API int roc_receiver_close(roc_receiver* receiver);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_RECEIVER_H_ */
