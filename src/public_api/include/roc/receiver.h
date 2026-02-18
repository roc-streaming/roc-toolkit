/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/receiver.h
 * \brief Receiver peer.
 */

#ifndef ROC_RECEIVER_H_
#define ROC_RECEIVER_H_

#include "roc/config.h"
#include "roc/context.h"
#include "roc/endpoint.h"
#include "roc/frame.h"
#include "roc/metrics.h"
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
 * - Optionally, the receiver parameters may be fine-tuned using roc_receiver_configure().
 *
 * - The receiver either binds local endpoints using roc_receiver_bind(), allowing senders
 *   connecting to them, or itself connects to remote sender endpoints using
 *   roc_receiver_connect(). What approach to use is up to the user.
 *
 * - The audio stream is iteratively read from the receiver using roc_receiver_read().
 *   Receiver returns the mixed stream from all connected senders.
 *
 * - The receiver is destroyed using roc_receiver_close().
 *
 * **Slots, interfaces, and endpoints**
 *
 * Receiver has one or multiple **slots**, which may be independently bound or connected.
 * Slots may be used to bind receiver to multiple addresses. Slots are numbered from
 * zero and are created automatically. In simple cases just use \c ROC_SLOT_DEFAULT.
 *
 * Each slot has its own set of *interfaces*, one per each type defined in \ref
 * roc_interface. The interface defines the type of the communication with the remote peer
 * and the set of the protocols supported by it.
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
 *   - Bind \ref ROC_INTERFACE_AGGREGATE to a local endpoint (e.g. be an RTSP server).
 *   - Connect \ref ROC_INTERFACE_AGGREGATE to a remote endpoint (e.g. be an RTSP
 *     client).
 *   - Bind \ref ROC_INTERFACE_AUDIO_SOURCE, \ref ROC_INTERFACE_AUDIO_REPAIR (optionally,
 *     for FEC), and \ref ROC_INTERFACE_AUDIO_CONTROL (optionally, for control messages)
 *     to local endpoints (e.g. be an RTP/FECFRAME/RTCP receiver).
 *
 * Slots can be removed using roc_receiver_unlink(). Removing a slot also removes all its
 * interfaces and terminates all associated connections.
 *
 * Slots can be added and removed at any time on fly and from any thread. It is safe
 * to do it from another thread concurrently with reading frames. Operations with
 * slots won't block concurrent reads.
 *
 * **FEC schemes**
 *
 * If \ref ROC_INTERFACE_AGGREGATE is used, it automatically creates all necessary
 * transport interfaces and the user should not bother about them.
 *
 * Otherwise, the user should manually configure \ref ROC_INTERFACE_AUDIO_SOURCE and
 * \ref ROC_INTERFACE_AUDIO_REPAIR interfaces:
 *
 *  - If FEC is disabled (\ref ROC_FEC_ENCODING_DISABLE), only
 *    \ref ROC_INTERFACE_AUDIO_SOURCE should be configured. It will be used to transmit
 *    audio packets.
 *
 *  - If FEC is enabled, both \ref ROC_INTERFACE_AUDIO_SOURCE and
 *    \ref ROC_INTERFACE_AUDIO_REPAIR interfaces should be configured. The second
 *    interface will be used to transmit redundant repair data.
 *
 * The protocols for the two interfaces should correspond to each other and to the FEC
 * scheme. For example, if \ref ROC_FEC_ENCODING_RS8M is used, the protocols should be
 * \ref ROC_PROTO_RTP_RS8M_SOURCE and \ref ROC_PROTO_RS8M_REPAIR.
 *
 * **Connections**
 *
 * Receiver creates a connection object for every sender connected to it. Connections can
 * appear and disappear at any time. Multiple connections can be active at the same time.
 *
 * A connection may contain multiple streams sent to different receiver ports. If the
 * sender employs FEC, connection usually has source, repair, and control streams.
 * Otherwise, connection usually has source and control streams.
 *
 * Connection is created automatically on the reception of the first packet from a new
 * sender, and terminated when there are no packets during a timeout. Connection can also
 * be terminated on other events like a large latency underrun or overrun or continuous
 * stuttering, but if the sender continues to send packets, connection will be created
 * again shortly.
 *
 * **Mixing**
 *
 * Receiver mixes audio streams from all currently active connections into a single output
 * stream.
 *
 * The output stream continues no matter how much active connections there are at the
 * moment. In particular, if there are no connections, the receiver produces a stream with
 * all zeros.
 *
 * Connections can be added and removed from the output stream at any time, probably in
 * the middle of a frame.
 *
 * **Transcoding**
 *
 * Every connection may have a different sample rate, channel layout, and encoding.
 *
 * Before mixing, receiver automatically transcodes all incoming streams to the format
 * of receiver frames.
 *
 * **Latency tuning and bounding**
 *
 * If latency tuning is enabled (which is by default enabled on receiver), receiver
 * monitors latency of each connection and adjusts per-connection clock to keep latency
 * close to the target value. The user can configure how the latency is measured, how
 * smooth is the tuning, and the target value.
 *
 * If latency bounding is enabled (which is also by default enabled on receiver), receiver
 * also ensures that latency lies within allowed boundaries, and terminates connection
 * otherwise. The user can configure those boundaries.
 *
 * To adjust connection clock, receiver uses resampling with a scaling factor slightly
 * above or below 1.0. Since resampling may be a quite time-consuming operation, the user
 * can choose between several resampler backends and profiles providing different
 * compromises between CPU consumption, quality, and precision.
 *
 * **Clock source**
 *
 * Receiver should decode samples at a constant rate that is configured when the receiver
 * is created. There are two ways to accomplish this:
 *
 *  - If the user enabled internal clock (\ref ROC_CLOCK_SOURCE_INTERNAL), the receiver
 *    employs a CPU timer to block reads until it's time to decode the next bunch of
 *    samples according to the configured sample rate.
 *
 *    This mode is useful when the user passes samples to a non-realtime destination,
 *    e.g. to an audio file.
 *
 *  - If the user enabled external clock (\ref ROC_CLOCK_SOURCE_EXTERNAL), the samples
 *    read from the receiver are decoded immediately and hence the user is responsible to
 *    call read operation according to the sample rate.
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
 *  - \p result should point to an uninitialized roc_receiver pointer
 *
 * **Returns**
 *  - returns zero if the receiver was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p config; it may be safely deallocated
 *    after the function returns
 *  - passes the ownership of \p result to the user; the user is responsible to call
 *    roc_receiver_close() to free it
 *  - attaches created receiver to \p context; the user should not close context
 *    before closing receiver
 */
ROC_API int roc_receiver_open(roc_context* context,
                              const roc_receiver_config* config,
                              roc_receiver** result);

/** Set receiver interface configuration.
 *
 * Updates configuration of specified interface of specified slot. If called, the
 * call should be done before calling roc_receiver_bind() or roc_receiver_connect()
 * for the same interface.
 *
 * Automatically initializes slot with given index if it's used first time.
 *
 * If an error happens during configure, the whole slot is disabled and marked broken.
 * The slot index remains reserved. The user is responsible for removing the slot
 * using roc_receiver_unlink(), after which slot index can be reused.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *  - \p slot specifies the receiver slot (if in doubt, use \c ROC_SLOT_DEFAULT)
 *  - \p iface specifies the receiver interface
 *  - \p config should be point to an initialized config
 *
 * **Returns**
 *  - returns zero if config was successfully updated
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if slot is already bound or connected
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p config; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_receiver_configure(roc_receiver* receiver,
                                   roc_slot slot,
                                   roc_interface iface,
                                   const roc_interface_config* config);

/** Bind the receiver interface to a local endpoint.
 *
 * Checks that the endpoint is valid and supported by the interface, allocates
 * a new ingoing port, and binds it to the local endpoint.
 *
 * Each slot's interface can be bound or connected only once.
 * May be called multiple times for different slots or interfaces.
 *
 * Automatically initializes slot with given index if it's used first time.
 *
 * If an error happens during bind, the whole slot is disabled and marked broken.
 * The slot index remains reserved. The user is responsible for removing the slot
 * using roc_receiver_unlink(), after which slot index can be reused.
 *
 * If \p endpoint has explicitly set zero port, the receiver is bound to a randomly
 * chosen ephemeral port. If the function succeeds, the actual port to which the
 * receiver was bound is written back to \p endpoint.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *  - \p slot specifies the receiver slot (if in doubt, use \c ROC_SLOT_DEFAULT)
 *  - \p iface specifies the receiver interface
 *  - \p endpoint specifies the receiver endpoint
 *
 * **Returns**
 *  - returns zero if the receiver was successfully bound to a port
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the address can't be bound
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p endpoint; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_receiver_bind(roc_receiver* receiver,
                              roc_slot slot,
                              roc_interface iface,
                              roc_endpoint* endpoint);

/** Query receiver slot metrics.
 *
 * Reads metrics into provided structs.
 *
 * To retrieve metrics of the slot as a whole, set \c slot_metrics to point to a single
 * \ref roc_receiver_metrics struct.
 *
 * To retrieve metrics of specific connections of the slot, set \c conn_metrics to point
 * to an array of \ref roc_connection_metrics structs, and \c conn_metrics_count to the
 * number of elements in the array. The function will write metrics to the array (no more
 * than array size) and update \c conn_metrics_count with the number of elements written.
 *
 * Actual number of connections (regardless of the array size) is also written to
 * \c connection_count field of \ref roc_receiver_metrics.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *  - \p slot specifies the receiver slot (if in doubt, use \c ROC_SLOT_DEFAULT)
 *  - \p slot_metrics defines a struct where to write slot metrics (may be NULL)
 *  - \p conn_metrics defines an array of structs where to write connection metrics
 *    (may be NULL)
 *  - \p conn_metrics_count defines number of elements in array
 *    (may be NULL if \c conn_metrics is NULL)
 *
 * **Returns**
 *  - returns zero if the metrics were successfully retrieved
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the slot does not exist
 *
 * **Ownership**
 *  - doesn't take or share the ownership of the provided buffers;
 *    they may be safely deallocated after the function returns
 */
ROC_API int roc_receiver_query(roc_receiver* receiver,
                               roc_slot slot,
                               roc_receiver_metrics* slot_metrics,
                               roc_connection_metrics* conn_metrics,
                               size_t* conn_metrics_count);

/** Delete receiver slot.
 *
 * Disconnects, unbinds, and removes all slot interfaces and removes the slot.
 * All associated connections to remote peers are properly terminated.
 *
 * After unlinking the slot, it can be re-created again by re-using slot index.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *  - \p slot specifies the receiver slot
 *
 * **Returns**
 *  - returns zero if the slot was successfully removed
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the slot does not exist
 */
ROC_API int roc_receiver_unlink(roc_receiver* receiver, roc_slot slot);

/** Read samples from the receiver.
 *
 * Reads retrieved network packets, decodes packets, repairs losses, extracts samples,
 * adjusts sample rate and channel layout, compensates clock drift, mixes samples from all
 * connections, and finally stores samples into the provided frame.
 *
 * If \ref ROC_CLOCK_SOURCE_INTERNAL is used, the function blocks until it's time to
 * decode the samples according to the configured sample rate.
 *
 * Until the receiver is connected to at least one sender, it produces silence.
 * If the receiver is connected to multiple senders, it mixes their streams into one.
 *
 * **Parameters**
 *  - \p receiver should point to an opened receiver
 *  - \p frame should point to an initialized frame; it should contain pointer to
 *    a buffer and it's size; the buffer is fully filled with data from receiver
 *
 * **Returns**
 *  - returns zero if all samples were successfully decoded
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p frame; it may be safely deallocated
 *    after the function returns
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
 *
 * **Ownership**
 *  - ends the user ownership of \p receiver; it can't be used anymore after the
 *    function returns
 */
ROC_API int roc_receiver_close(roc_receiver* receiver);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_RECEIVER_H_ */
