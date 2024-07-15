/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/sender.h
 * \brief Sender peer.
 */

#ifndef ROC_SENDER_H_
#define ROC_SENDER_H_

#include "roc/config.h"
#include "roc/context.h"
#include "roc/endpoint.h"
#include "roc/frame.h"
#include "roc/metrics.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Sender peer.
 *
 * Sender gets an audio stream from the user, encodes it into network packets, and
 * transmits them to a remote receiver.
 *
 * **Context**
 *
 * Sender is automatically attached to a context when opened and detached from it when
 * closed. The user should not close the context until the sender is closed.
 *
 * Sender work consists of two parts: stream encoding and packet transmission. The
 * encoding part is performed in the sender itself, and the transmission part is
 * performed in the context network worker threads.
 *
 * **Life cycle**
 *
 * - A sender is created using roc_sender_open().
 *
 * - Optionally, the sender parameters may be fine-tuned using roc_sender_configure().
 *
 * - The sender either binds local endpoints using roc_sender_bind(), allowing receivers
 *   connecting to them, or itself connects to remote receiver endpoints using
 *   roc_sender_connect(). What approach to use is up to the user.
 *
 * - The audio stream is iteratively written to the sender using roc_sender_write(). The
 *   sender encodes the stream into packets and send to connected receiver(s).
 *
 * - The sender is destroyed using roc_sender_close().
 *
 * **Slots, interfaces, and endpoints**
 *
 * Sender has one or multiple **slots**, which may be independently bound or connected.
 * Slots may be used to connect sender to multiple receivers. Slots are numbered from
 * zero and are created automatically. In simple cases just use \c ROC_SLOT_DEFAULT.
 *
 * Each slot has its own set of *interfaces*, one per each type defined in \ref
 * roc_interface. The interface defines the type of the communication with the remote peer
 * and the set of the protocols supported by it.
 *
 * Supported actions with the interface:
 *
 *  - Call roc_sender_bind() to bind the interface to a local \ref roc_endpoint. In this
 *    case the sender accepts connections from receivers and sends media stream to all
 *    connected receivers.
 *
 *  - Call roc_sender_connect() to connect the interface to a remote \ref roc_endpoint.
 *    In this case the sender initiates connection to the receiver and starts sending
 *    media stream to it.
 *
 * Supported interface configurations:
 *
 *   - Connect \ref ROC_INTERFACE_AGGREGATE to a remote endpoint (e.g. be an RTSP
 *     client).
 *   - Bind \ref ROC_INTERFACE_AGGREGATE to a local endpoint (e.g. be an RTSP server).
 *   - Connect \ref ROC_INTERFACE_AUDIO_SOURCE, \ref ROC_INTERFACE_AUDIO_REPAIR
 *     (optionally, for FEC), and \ref ROC_INTERFACE_AUDIO_CONTROL (optionally, for
 *     control messages) to remote endpoints (e.g. be an RTP/FECFRAME/RTCP sender).
 *
 * Slots can be removed using roc_sender_unlink(). Removing a slot also removes all its
 * interfaces and terminates all associated connections.
 *
 * Slots can be added and removed at any time on fly and from any thread. It is safe
 * to do it from another thread concurrently with writing frames. Operations with
 * slots won't block concurrent writes.
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
 * **Transcoding**
 *
 * If encoding of sender frames and network packets are different, sender automatically
 * performs all necessary transcoding.
 *
 * **Latency tuning and bounding**
 *
 * Usually latency tuning and bounding is done on receiver side, but it's possible to
 * disable it on receiver and enable on sender. It is useful if receiver is does not
 * support it or does not have enough CPU to do it with good quality. This feature
 * requires use of \ref ROC_PROTO_RTCP to deliver necessary latency metrics from
 * receiver to sender.
 *
 * If latency tuning is enabled (which is by default disabled on sender), sender
 * monitors latency and adjusts connection clock to keep latency close to the target
 * value. The user can configure how the latency is measured, how smooth is the tuning,
 * and the target value.
 *
 * If latency bounding is enabled (which is also by default disabled on sender), sender
 * also  ensures that latency lies within allowed boundaries, and restarts connection
 * otherwise. The user can configure those boundaries.
 *
 * To adjust connection clock, sender uses resampling with a scaling factor slightly
 * above or below 1.0. Since resampling may be a quite time-consuming operation, the user
 * can choose between several resampler backends and profiles providing different
 * compromises between CPU consumption, quality, and precision.
 *
 * **Clock source**
 *
 * Sender should encode samples at a constant rate that is configured when the sender
 * is created. There are two ways to accomplish this:
 *
 *  - If the user enabled internal clock (\ref ROC_CLOCK_SOURCE_INTERNAL), the sender
 *    employs a CPU timer to block writes until it's time to encode the next bunch of
 *    samples according to the configured sample rate.

 *    This mode is useful when the user gets samples from a non-realtime source, e.g.
 *    from an audio file.
 *
 *  - If the user enabled external clock (\ref ROC_CLOCK_SOURCE_EXTERNAL), the samples
 *    written to the sender are encoded and sent immediately, and hence the user is
 *    responsible to call write operation according to the sample rate.
 *
 *    This mode is useful when the user gets samples from a realtime source with its own
 *    clock, e.g. from an audio device. Internal clock should not be used in this case
 *    because the audio device and the CPU might have slightly different clocks, and the
 *    difference will eventually lead to an underrun or an overrun.
 *
 * **Thread safety**
 *
 * Can be used concurrently.
 */
typedef struct roc_sender roc_sender;

/** Open a new sender.
 *
 * Allocates and initializes a new sender, and attaches it to the context.
 *
 * **Parameters**
 *  - \p context should point to an opened context
 *  - \p config should point to an initialized config
 *  - \p result should point to an uninitialized roc_sender pointer
 *
 * **Returns**
 *  - returns zero if the sender was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p config; it may be safely deallocated
 *    after the function returns
 *  - passes the ownership of \p result to the user; the user is responsible to call
 *    roc_sender_close() to free it
 *  - attaches created sender to \p context; the user should not close context
 *    before closing sender
 */
ROC_API int roc_sender_open(roc_context* context,
                            const roc_sender_config* config,
                            roc_sender** result);

/** Set sender interface configuration.
 *
 * Updates configuration of specified interface of specified slot. If called, the
 * call should be done before calling roc_sender_bind() or roc_sender_connect()
 * for the same interface.
 *
 * Automatically initializes slot with given index if it's used first time.
 *
 * If an error happens during configure, the whole slot is disabled and marked broken.
 * The slot index remains reserved. The user is responsible for removing the slot
 * using roc_sender_unlink(), after which slot index can be reused.
 *
 * **Parameters**
 *  - \p sender should point to an opened sender
 *  - \p slot specifies the sender slot index (if in doubt, use \c ROC_SLOT_DEFAULT)
 *  - \p iface specifies the sender interface
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
ROC_API int roc_sender_configure(roc_sender* sender,
                                 roc_slot slot,
                                 roc_interface iface,
                                 const roc_interface_config* config);

/** Connect the sender interface to a remote receiver endpoint.
 *
 * Checks that the endpoint is valid and supported by the interface, allocates
 * a new outgoing port, and connects it to the remote endpoint.
 *
 * Each slot's interface can be bound or connected only once.
 * May be called multiple times for different slots or interfaces.
 *
 * Automatically initializes slot with given index if it's used first time.
 *
 * If an error happens during connect, the whole slot is disabled and marked broken.
 * The slot index remains reserved. The user is responsible for removing the slot
 * using roc_sender_unlink(), after which slot index can be reused.
 *
 * **Parameters**
 *  - \p sender should point to an opened sender
 *  - \p slot specifies the sender slot (if in doubt, use \c ROC_SLOT_DEFAULT)
 *  - \p iface specifies the sender interface
 *  - \p endpoint specifies the receiver endpoint
 *
 * **Returns**
 *  - returns zero if the sender was successfully connected
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p endpoint; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_sender_connect(roc_sender* sender,
                               roc_slot slot,
                               roc_interface iface,
                               const roc_endpoint* endpoint);

/** Query sender slot metrics.
 *
 * Reads metrics into provided structs.
 *
 * To retrieve metrics of the slot as a whole, set \c slot_metrics to point to a single
 * \ref roc_sender_metrics struct.
 *
 * To retrieve metrics of specific connections of the slot, set \c conn_metrics to point
 * to an array of \ref roc_connection_metrics structs, and \c conn_metrics_count to the
 * number of elements in the array. The function will write metrics to the array (no more
 * than array size) and update \c conn_metrics_count with the number of elements written.
 *
 * Actual number of connections (regardless of the array size) is also written to
 * \c connection_count field of \ref roc_sender_metrics.
 *
 * **Parameters**
 *  - \p sender should point to an opened sender
 *  - \p slot specifies the sender slot (if in doubt, use \c ROC_SLOT_DEFAULT)
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
ROC_API int roc_sender_query(roc_sender* sender,
                             roc_slot slot,
                             roc_sender_metrics* slot_metrics,
                             roc_connection_metrics* conn_metrics,
                             size_t* conn_metrics_count);

/** Delete sender slot.
 *
 * Disconnects, unbinds, and removes all slot interfaces and removes the slot.
 * All associated connections to remote peers are properly terminated.
 *
 * After unlinking the slot, it can be re-created again by re-using slot index.
 *
 * **Parameters**
 *  - \p sender should point to an opened sender
 *  - \p slot specifies the sender slot
 *
 * **Returns**
 *  - returns zero if the slot was successfully removed
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the slot does not exist
 */
ROC_API int roc_sender_unlink(roc_sender* sender, roc_slot slot);

/** Encode samples to packets and transmit them to the receiver.
 *
 * Encodes samples to packets and enqueues them for transmission by the network worker
 * thread of the context.
 *
 * If \ref ROC_CLOCK_SOURCE_INTERNAL is used, the function blocks until it's time to
 * transmit the samples according to the configured sample rate. The function returns
 * after encoding and enqueuing the packets, without waiting when the packets are actually
 * transmitted.
 *
 * Until the sender is connected to at least one receiver, the stream is just dropped.
 * If the sender is connected to multiple receivers, the stream is duplicated to
 * each of them.
 *
 * **Parameters**
 *  - \p sender should point to an opened sender
 *  - \p frame should point to an initialized frame; it should contain pointer to
 *    a buffer and it's size; the buffer is fully copied into the sender
 *
 * **Returns**
 *  - returns zero if all samples were successfully encoded and enqueued
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p frame; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_sender_write(roc_sender* sender, const roc_frame* frame);

/** Close the sender.
 *
 * Deinitializes and deallocates the sender, and detaches it from the context. The user
 * should ensure that nobody uses the sender during and after this call. If this
 * function fails, the sender is kept opened and attached to the context.
 *
 * **Parameters**
 *  - \p sender should point to an opened sender
 *
 * **Returns**
 *  - returns zero if the sender was successfully closed
 *  - returns a negative value if the arguments are invalid
 *
 * **Ownership**
 *  - ends the user ownership of \p sender; it can't be used anymore after the
 *    function returns
 */
ROC_API int roc_sender_close(roc_sender* sender);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_SENDER_H_ */
