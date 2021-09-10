/* Send and receive samples using bare RTP.
 *
 * This example creates a receiver and binds it to an RTP endpoint.
 * Then it creates a sender and connects it to the receiver endpoint.
 * Then it starts writing audio stream to the sender and reading it from receiver.
 *
 * Building:
 *   cc send_receive_rtp.c -lroc
 *
 * Running:
 *   ./a.out
 *
 * License:
 *   public domain
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/endpoint.h>
#include <roc/log.h>
#include <roc/receiver.h>
#include <roc/sender.h>

/* Receiver parameters. */
#define MY_RECEIVER_IP "127.0.0.1"
#define MY_RECEIVER_SOURCE_PORT 10201
#define MY_RECEIVER_REPAIR_PORT 10202

/* Signal parameters */
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

    receiver_config.frame_sample_rate = MY_SAMPLE_RATE;
    receiver_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    receiver_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    /* Receiver should clock itself. */
    receiver_config.clock_source = ROC_CLOCK_INTERNAL;

    roc_receiver* receiver = NULL;
    if (roc_receiver_open(context, &receiver_config, &receiver) != 0) {
        oops();
    }

    /* Bind receiver to the source (audio) packets endpoint.
     * The receiver will expect packets with RTP header on this port. */
    roc_endpoint* source_endp = NULL;
    if (roc_endpoint_allocate(&source_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(source_endp, ROC_PROTO_RTP);
    roc_endpoint_set_host(source_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(source_endp, MY_RECEIVER_SOURCE_PORT);

    if (roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endp) != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(source_endp) != 0) {
        oops();
    }

    /* Read samples from the receiver. */
    for (;;) {
        float samples[MY_BUFFER_SIZE];
        memset(samples, 0, sizeof(samples));

        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_receiver_read(receiver, &frame) != 0) {
            break;
        }

        /* Check whether the frame has zero samples.
         * Since the sender in this example produces only non-zero samples, a zero
         * means that the sender is either not (yet) connected or a packet was lost. */
        int frame_has_zeros = 0;
        int i;
        for (i = 0; i < MY_BUFFER_SIZE; i++) {
            if (samples[i] < 1e9f) {
                frame_has_zeros = 1;
                break;
            }
        }

        printf("%c", frame_has_zeros ? 'z' : '.');
        fflush(stdout);
    }

    if (roc_receiver_close(receiver) != 0) {
        oops();
    }

    return NULL;
}

static void* sender_loop(void* arg) {
    roc_context* context = (roc_context*)arg;

    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    sender_config.frame_sample_rate = MY_SAMPLE_RATE;
    sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    /* Sender should not use any FEC scheme. */
    sender_config.fec_code = ROC_FEC_DISABLE;

    /* Sender should clock itself. */
    sender_config.clock_source = ROC_CLOCK_INTERNAL;

    roc_sender* sender = NULL;
    if (roc_sender_open(context, &sender_config, &sender) != 0) {
        oops();
    }

    /* Connect sender to the receiver source (audio) packets endpoint.
     * The receiver should expect packets with RTP header on that port. */
    roc_endpoint* source_endp = NULL;
    if (roc_endpoint_allocate(&source_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(source_endp, ROC_PROTO_RTP);
    roc_endpoint_set_host(source_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(source_endp, MY_RECEIVER_SOURCE_PORT);

    if (roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endp) != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(source_endp) != 0) {
        oops();
    }

    /* Prepare some non-zero samples. */
    float samples[MY_BUFFER_SIZE];
    int i;
    for (i = 0; i < MY_BUFFER_SIZE; i++) {
        samples[i] = 0.5f;
    }

    /* Write samples to the sender. */
    for (;;) {
        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_sender_write(sender, &frame) != 0) {
            break;
        }
    }

    if (roc_sender_close(sender) != 0) {
        oops();
    }

    return NULL;
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

    /* Wont happen. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
