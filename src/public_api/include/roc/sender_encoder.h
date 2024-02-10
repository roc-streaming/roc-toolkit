/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/sender_encoder.h
 * \brief Roc sender encoder.
 */

#ifndef ROC_SENDER_ENCODER_H_
#define ROC_SENDER_ENCODER_H_

#include "roc/config.h"
#include "roc/context.h"
#include "roc/frame.h"
#include "roc/metrics.h"
#include "roc/packet.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Sender encoder.
 *
 * Sender encoder gets an audio stream from the user, encodes it into network packets, and
 * provides encoded packets to the user.
 *
 * Sender encoder is a networkless version of \ref roc_sender. It implements the same
 * pipeline, but instead of sending packets to network, it returns them to the user. The
 * user is responsible for carrying packets over network.
 *
 * For detailed description of sender pipeline, see documentation for \ref roc_sender.
 *
 * **Life cycle**
 *
 * - Encoder is created using roc_sender_encoder_open().
 *
 * - The user activates one or more interfaces by invoking roc_sender_encoder_activate().
 *   This tells encoder what types of streams to produces and what protocols to use for
 *   them (e.g. only audio packets or also redundancy packets).
 *
 * - The audio stream is iteratively pushed to the encoder using
 *   roc_sender_encoder_push_frame(). The sender encodes the stream into packets and
 *   accumulates them in internal queue.
 *
 * - The packet stream is iteratively popped from the encoder internal queue using
 *   roc_sender_encoder_pop_packet(). User should retrieve all available packets from all
 *   activated interfaces every time after pushing a frame.
 *
 * - User is responsible for delivering packets to \ref roc_receiver_decoder and pushing
 *   them to appropriate interfaces of decoder.
 *
 * - In addition, if a control interface is activated, the stream of encoded feedback
 *   packets from decoder is pushed to encoder internal queue using
 *   roc_sender_encoder_push_feedback_packet().
 *
 * - User is responsible for delivering feedback packets from \ref roc_receiver_decoder
 *   and pushing them to appropriate interfaces of encoder.
 *
 * - Encoder is eventually destroyed using roc_sender_encoder_close().
 *
 * **Interfaces and protocols**
 *
 * Sender encoder may have one or several *interfaces*, as defined in \ref roc_interface.
 * The interface defines the type of the communication with the remote peer and the set
 * of the protocols supported by it.
 *
 * Each interface has its own outbound packet queue. When a frame is pushed to the
 * encoder, it may produce multiple packets for each interface queue. The user then should
 * pop packets from each interface that was activated.
 *
 * **Feedback packets**
 *
 * Control interface in addition has inbound packet queue. The user should push feedback
 * packets from decoder to this queue. When a frame is pushed to encoder, it consumes
 * those accumulated packets.
 *
 * The user should deliver feedback packets from decoder back to encoder. Feedback packets
 * allow decoder and encoder to exchange metrics like latency and losses, and several
 * features like latency calculations require feedback to function properly.
 *
 * **Thread safety**
 *
 * Can be used concurrently.
 */
typedef struct roc_sender_encoder roc_sender_encoder;

/** Open a new encoder.
 *
 * Allocates and initializes a new encoder, and attaches it to the context.
 *
 * **Parameters**
 *  - \p context should point to an opened context
 *  - \p config should point to an initialized config
 *  - \p result should point to an unitialized roc_sender_encoder pointer
 *
 * **Returns**
 *  - returns zero if the encoder was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p config; it may be safely deallocated
 *    after the function returns
 *  - passes the ownership of \p result to the user; the user is responsible to call
 *    roc_sender_encoder_close() to free it
 *  - attaches created encoder to \p context; the user should not close context
 *    before closing encoder
 */
ROC_API int roc_sender_encoder_open(roc_context* context,
                                    const roc_sender_config* config,
                                    roc_sender_encoder** result);

/** Activate encoder interface.
 *
 * Checks that the protocol is valid and supported by the interface, and
 * initializes given interface with given protocol.
 *
 * The user should invoke roc_sender_encoder_pop_packet() for all activated interfaces
 * and deliver packets to appropriate interfaces of \ref roc_receiver_decoder.
 *
 * **Parameters**
 *  - \p encoder should point to an opened encoder
 *  - \p iface specifies the encoder interface
 *  - \p proto specifies the encoder protocol
 *
 * **Returns**
 *  - returns zero if interface was successfully activated
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 */
ROC_API int roc_sender_encoder_activate(roc_sender_encoder* encoder,
                                        roc_interface iface,
                                        roc_protocol proto);

/** Query encoder metrics.
 *
 * Reads metrics into provided structs.
 *
 * To retrieve metrics of the encoder as a whole, set \c slot_metrics to point to a single
 * \ref roc_sender_metrics struct.
 *
 * To retrieve metrics of specific connections of the encoder, set \c conn_metrics to
 * point to an array of \ref roc_connection_metrics structs, and \c conn_metrics_count to
 * the number of elements in the array. The function will write metrcis to the array (no
 * more than array size) and update \c conn_metrics_count with the number of elements
 * written.
 *
 * Actual number of connections (regardless of the array size) is also written to
 * \c connection_count field of \ref roc_sender_metrics.
 *
 * **Parameters**
 *  - \p sender should point to an opened sender
 *  - \p encoder_metrics defines a struct where to write encoder metrics (may be NULL)
 *  - \p conn_metrics defines an array of structs where to write connection metrics
 *    (may be NULL)
 *  - \p conn_metrics_count defines number of elements in array
 *    (may be NULL if \c conn_metrics is NULL)
 *
 * **Returns**
 *  - returns zero if the metrics were successfully retrieved
 *  - returns a negative value if the arguments are invalid
 *
 * **Ownership**
 *  - doesn't take or share the ownership of the provided buffers;
 *    they may be safely deallocated after the function returns
 */
ROC_API int roc_sender_encoder_query(roc_sender_encoder* encoder,
                                     roc_sender_metrics* encoder_metrics,
                                     roc_connection_metrics* conn_metrics,
                                     size_t* conn_metrics_count);

/** Write frame to encoder.
 *
 * Encodes samples to into network packets and enqueues them to internal queues of
 * activated interfaces.
 *
 * If \ref ROC_CLOCK_SOURCE_INTERNAL is used, the function blocks until it's time to
 * encode the samples according to the configured sample rate.
 *
 * Until at least one interface is activated, the stream is just dropped.
 *
 * **Parameters**
 *  - \p encoder should point to an opened encoder
 *  - \p frame should point to an initialized frame; it should contain pointer to
 *    a buffer and it's size; the buffer is fully copied into encoder
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
ROC_API int roc_sender_encoder_push_frame(roc_sender_encoder* encoder,
                                          const roc_frame* frame);

/** Write feedback packet to encoder.
 *
 * Adds encoded feedback packet to the interface queue.
 *
 * The user should iteratively push all delivered feedback packets to appropriate
 * interfaces. They will be later consumed by roc_sender_encoder_push_frame().
 *
 * **Parameters**
 *  - \p encoder should point to an opened encoder
 *  - \p packet should point to an initialized packet; it should contain pointer to
 *    a buffer and it's size; the buffer is fully copied into encoder
 *
 * **Returns**
 *  - returns zero if a packet was successfully copied to encoder
 *  - returns a negative value if the interface is not activated
 *  - returns a negative value if the buffer size of the provided packet is too large
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p packet; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_sender_encoder_push_feedback_packet(roc_sender_encoder* encoder,
                                                    roc_interface iface,
                                                    const roc_packet* packet);

/** Read packet from encoder.
 *
 * Removes encoded packet from interface queue and returns it to the user.
 *
 * Packets are added to the queue from roc_sender_encoder_push_frame(). Each push may
 * produce multiple packets, so the user should iteratively pop packets until error.
 * This should be repeated for all activated interfaces.
 *
 * **Parameters**
 *  - \p encoder should point to an opened encoder
 *  - \p packet should point to an initialized packet; it should contain pointer to
 *    a buffer and it's size; packet bytes are copied to user's buffer and the
 *    size field is updated with the actual packet size
 *
 * **Returns**
 *  - returns zero if a packet was successfully copied from encoder
 *  - returns a negative value if there are no more packets for this interface
 *  - returns a negative value if the interface is not activated
 *  - returns a negative value if the buffer size of the provided packet is too small
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p packet; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_sender_encoder_pop_packet(roc_sender_encoder* encoder,
                                          roc_interface iface,
                                          roc_packet* packet);

/** Close encoder.
 *
 * Deinitializes and deallocates the encoder, and detaches it from the context. The user
 * should ensure that nobody uses the encoder during and after this call. If this
 * function fails, the encoder is kept opened and attached to the context.
 *
 * **Parameters**
 *  - \p encoder should point to an opened encoder
 *
 * **Returns**
 *  - returns zero if the encoder was successfully closed
 *  - returns a negative value if the arguments are invalid
 *
 * **Ownership**
 *  - ends the user ownership of \p encoder; it can't be used anymore after the
 *    function returns
 */
ROC_API int roc_sender_encoder_close(roc_sender_encoder* encoder);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_SENDER_ENCODER_H_ */
