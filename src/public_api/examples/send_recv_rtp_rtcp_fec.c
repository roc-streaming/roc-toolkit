/*
 * This example does sending and receiving using RTP + FECFRAME + RTCP.
 *
 * It is similar to send_recv_rtp.c, but it creates three endpoints:
 *  - source endpoint is used to transmit audio stream
 *  - repair endpoint is used to transmit redundant stream for loss recovery
 *  - control endpoint is used to transmit bidirectional control traffic
 *
 * Building:
 *   cc -o send_recv_rtp_rtcp_fec send_recv_rtp_rtcp_fec.c -lroc
 *
 * Running:
 *   ./send_recv_rtp_rtcp_fec
 *
 * License:
 *   public domain
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/log.h>
#include <roc/receiver.h>
#include <roc/sender.h>

/* Network parameters. */
#define MY_RECEIVER_SOURCE_ENDPOINT "rtp+rs8m://127.0.0.1:10201"
#define MY_RECEIVER_REPAIR_ENDPOINT "rs8m://127.0.0.1:10202"
#define MY_RECEIVER_CONTROL_ENDPOINT "rtcp://127.0.0.1:10203"

/* Audio parameters. */
#define MY_SAMPLE_RATE 44100
#define MY_BUFFER_SIZE 2000

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

static void* receiver_loop(void* arg) {
    roc_context* context = (roc_context*)arg;

    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    receiver_config.frame_encoding.format = ROC_FORMAT_PCM;
    receiver_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
    receiver_config.frame_encoding.rate = MY_SAMPLE_RATE;
    receiver_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

    /* Make read operation blocking as we don't have our own clock. */
    receiver_config.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

    roc_receiver* receiver = NULL;
    if (roc_receiver_open(context, &receiver_config, &receiver) != 0) {
        oops();
    }

    /* Bind receiver to the source (audio) packets endpoint.
     * The receiver will expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on this port. */
    roc_endpoint* source_endp = NULL;
    roc_endpoint_allocate(&source_endp);
    roc_endpoint_set_uri(source_endp, MY_RECEIVER_SOURCE_ENDPOINT);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                          source_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(source_endp);

    /* Bind receiver to the repair (FEC) packets endpoint.
     * The receiver will expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on this port. */
    roc_endpoint* repair_endp = NULL;
    roc_endpoint_allocate(&repair_endp);
    roc_endpoint_set_uri(repair_endp, MY_RECEIVER_REPAIR_ENDPOINT);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_REPAIR,
                          repair_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(repair_endp);

    /* Bind receiver to the control (RTCP) packets endpoint. */
    roc_endpoint* control_endp = NULL;
    roc_endpoint_allocate(&control_endp);
    roc_endpoint_set_uri(control_endp, MY_RECEIVER_CONTROL_ENDPOINT);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_CONTROL,
                          control_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(control_endp);

    /* Read samples from the receiver. */
    for (unsigned long nf = 0;; nf++) {
        float samples[MY_BUFFER_SIZE];

        roc_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_receiver_read(receiver, &frame) != 0) {
            break;
        }

        /* Here we can process received samples */
        if (nf % 100 == 0) {
            printf(">>> receiver frame counter: %lu\n", nf);
        }
    }

    if (roc_receiver_close(receiver) != 0) {
        oops();
    }

    return NULL;
}

static void sender_loop(roc_context* context) {
    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    sender_config.frame_encoding.format = ROC_FORMAT_PCM;
    sender_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
    sender_config.frame_encoding.rate = MY_SAMPLE_RATE;
    sender_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

    /* Enable Reed-Solomon FEC scheme because we use ROC_PROTO_RTP_RS8M_SOURCE
     * and ROC_PROTO_RS8M_REPAIR protocols. */
    sender_config.fec_encoding = ROC_FEC_ENCODING_RS8M;
    sender_config.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;

    /* Make write operation blocking as we don't have our own clock. */
    sender_config.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

    roc_sender* sender = NULL;
    if (roc_sender_open(context, &sender_config, &sender) != 0) {
        oops();
    }

    /* Connect sender to the receiver source (audio) packets endpoint.
     * The receiver should expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on that port. */
    roc_endpoint* source_endp = NULL;
    roc_endpoint_allocate(&source_endp);
    roc_endpoint_set_uri(source_endp, MY_RECEIVER_SOURCE_ENDPOINT);

    if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                           source_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(source_endp);

    /* Connect sender to the receiver repair (FEC) packets endpoint.
     * The receiver should expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on that port. */
    roc_endpoint* repair_endp = NULL;
    roc_endpoint_allocate(&repair_endp);
    roc_endpoint_set_uri(repair_endp, MY_RECEIVER_REPAIR_ENDPOINT);

    if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_REPAIR,
                           repair_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(repair_endp);

    /* Connect sender to the receiver control (RTCP) packets endpoint. */
    roc_endpoint* control_endp = NULL;
    roc_endpoint_allocate(&control_endp);
    roc_endpoint_set_uri(control_endp, MY_RECEIVER_CONTROL_ENDPOINT);

    if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_CONTROL,
                           control_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(control_endp);

    /* Write samples to the sender. */
    for (unsigned long nf = 0;; nf++) {
        /* Here we can fill samples to be sent */
        float samples[MY_BUFFER_SIZE];
        memset(samples, 0, sizeof(samples));

        roc_frame frame;
        memset(&frame, 0xff, sizeof(frame));
        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_sender_write(sender, &frame) != 0) {
            break;
        }

        if (nf % 100 == 0) {
            printf(">>> sender frame counter: %lu\n", nf);
        }
    }

    if (roc_sender_close(sender) != 0) {
        oops();
    }
}

int main() {
    roc_log_set_level(ROC_LOG_INFO);

    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* We use a single context for sender and receiver.
     * But it is possible to use separate contexts as well. */
    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Run receiver in separate thread. */
    pthread_t receiver_thread;
    if (pthread_create(&receiver_thread, NULL, receiver_loop, context) != 0) {
        oops();
    }

    /* Run sender in main thread. */
    sender_loop(context);

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
