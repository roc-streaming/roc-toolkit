/*
 * This example shows how to use slots mechanism to connect sender to 2 different
 * receivers using unicast addresses.
 *
 * Flow:
 *   - creates two receivers and binds each one to its own unicast address
 *   - creates a sender
 *   - connects slot 1 of the sender to the first receiver
 *   - connects slot 2 of the sender to the second receiver
 *   - one thread writes audio stream to the sender
 *   - another two threads read audio stream from receivers
 *
 * Building:
 *   cc -o send_recv_1_sender_2_receivers send_recv_1_sender_2_receivers.c -lroc
 *
 * Running:
 *   ./send_recv_1_sender_2_receivers
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

/* First receiver has 3 endpoints. */
#define MY_RECEIVER_1_SOURCE_ENDPOINT "rtp+rs8m://127.0.0.1:10201"
#define MY_RECEIVER_1_REPAIR_ENDPOINT "rs8m://127.0.0.1:10202"
#define MY_RECEIVER_1_CONTROL_ENDPOINT "rtcp://127.0.0.1:10203"

/* Second receiver also has 3 endpoints. */
#define MY_RECEIVER_2_SOURCE_ENDPOINT "rtp+rs8m://127.0.0.1:10301"
#define MY_RECEIVER_2_REPAIR_ENDPOINT "rs8m://127.0.0.1:10302"
#define MY_RECEIVER_2_CONTROL_ENDPOINT "rtcp://127.0.0.1:10303"

/* Sender slot identifiers, can be arbitrary numbers. */
#define MY_SENDER_SLOT_1 1
#define MY_SENDER_SLOT_2 2

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
    long receiver_id = (long)arg;

    const char* source_uri = NULL;
    const char* repair_uri = NULL;
    const char* control_uri = NULL;

    if (receiver_id == 1) {
        source_uri = MY_RECEIVER_1_SOURCE_ENDPOINT;
        repair_uri = MY_RECEIVER_1_REPAIR_ENDPOINT;
        control_uri = MY_RECEIVER_1_CONTROL_ENDPOINT;
    } else {
        source_uri = MY_RECEIVER_2_SOURCE_ENDPOINT;
        repair_uri = MY_RECEIVER_2_REPAIR_ENDPOINT;
        control_uri = MY_RECEIVER_2_CONTROL_ENDPOINT;
    }

    /* Create context. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Create receiver. */
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

    /* Bind receiver to three endpoints.
     */
    roc_endpoint* source_endp = NULL;
    roc_endpoint_allocate(&source_endp);
    roc_endpoint_set_uri(source_endp, source_uri);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                          source_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(source_endp);

    roc_endpoint* repair_endp = NULL;
    roc_endpoint_allocate(&repair_endp);
    roc_endpoint_set_uri(repair_endp, repair_uri);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_REPAIR,
                          repair_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(repair_endp);

    roc_endpoint* control_endp = NULL;
    roc_endpoint_allocate(&control_endp);
    roc_endpoint_set_uri(control_endp, control_uri);

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
            printf(">>> receiver %ld frame counter: %lu\n", receiver_id, nf);
        }
    }

    /* Destroy receiver and context. */
    if (roc_receiver_close(receiver) != 0) {
        oops();
    }
    if (roc_context_close(context) != 0) {
        oops();
    }

    return NULL;
}

static void sender_loop() {
    /* Create context. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Create sender. */
    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    sender_config.frame_encoding.format = ROC_FORMAT_PCM;
    sender_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
    sender_config.frame_encoding.rate = MY_SAMPLE_RATE;
    sender_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

    sender_config.fec_encoding = ROC_FEC_ENCODING_RS8M;
    sender_config.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;

    /* Make write operation blocking as we don't have our own clock. */
    sender_config.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

    roc_sender* sender = NULL;
    if (roc_sender_open(context, &sender_config, &sender) != 0) {
        oops();
    }

    /* Connect three endpoints of MY_SENDER_SLOT_1 to receiver 1.
     *
     * Note that there is no need to explicitly create slot, we just pass slot id
     * to roc_sender_connect() and the slot is created automatically.
     */
    roc_endpoint* slot_1_source_endp = NULL;
    roc_endpoint_allocate(&slot_1_source_endp);
    roc_endpoint_set_uri(slot_1_source_endp, MY_RECEIVER_1_SOURCE_ENDPOINT);

    if (roc_sender_connect(sender, MY_SENDER_SLOT_1, ROC_INTERFACE_AUDIO_SOURCE,
                           slot_1_source_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_1_source_endp);

    roc_endpoint* slot_1_repair_endp = NULL;
    roc_endpoint_allocate(&slot_1_repair_endp);
    roc_endpoint_set_uri(slot_1_repair_endp, MY_RECEIVER_1_REPAIR_ENDPOINT);

    if (roc_sender_connect(sender, MY_SENDER_SLOT_1, ROC_INTERFACE_AUDIO_REPAIR,
                           slot_1_repair_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_1_repair_endp);

    roc_endpoint* slot_1_control_endp = NULL;
    roc_endpoint_allocate(&slot_1_control_endp);
    roc_endpoint_set_uri(slot_1_control_endp, MY_RECEIVER_1_CONTROL_ENDPOINT);

    if (roc_sender_connect(sender, MY_SENDER_SLOT_1, ROC_INTERFACE_AUDIO_CONTROL,
                           slot_1_control_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_1_control_endp);

    /* Connect three endpoints of MY_SENDER_SLOT_2 to receiver 2.
     *
     * Again, we don't need to create slot explicitly.
     */
    roc_endpoint* slot_2_source_endp = NULL;
    roc_endpoint_allocate(&slot_2_source_endp);
    roc_endpoint_set_uri(slot_2_source_endp, MY_RECEIVER_2_SOURCE_ENDPOINT);

    if (roc_sender_connect(sender, MY_SENDER_SLOT_2, ROC_INTERFACE_AUDIO_SOURCE,
                           slot_2_source_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_2_source_endp);

    roc_endpoint* slot_2_repair_endp = NULL;
    roc_endpoint_allocate(&slot_2_repair_endp);
    roc_endpoint_set_uri(slot_2_repair_endp, MY_RECEIVER_2_REPAIR_ENDPOINT);

    if (roc_sender_connect(sender, MY_SENDER_SLOT_2, ROC_INTERFACE_AUDIO_REPAIR,
                           slot_2_repair_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_2_repair_endp);

    roc_endpoint* slot_2_control_endp = NULL;
    roc_endpoint_allocate(&slot_2_control_endp);
    roc_endpoint_set_uri(slot_2_control_endp, MY_RECEIVER_2_CONTROL_ENDPOINT);

    if (roc_sender_connect(sender, MY_SENDER_SLOT_2, ROC_INTERFACE_AUDIO_CONTROL,
                           slot_2_control_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_2_control_endp);

    /* Write samples to the sender. */
    for (unsigned long nf = 0;; nf++) {
        /* Here we can fill samples to be sent */
        float samples[MY_BUFFER_SIZE];
        memset(samples, 0xff, sizeof(samples));

        roc_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_sender_write(sender, &frame) != 0) {
            break;
        }

        if (nf % 100 == 0) {
            printf(">>> sender frame counter: %lu\n", nf);
        }
    }

    /* Destroy sender and context. */
    if (roc_sender_close(sender) != 0) {
        oops();
    }
    if (roc_context_close(context) != 0) {
        oops();
    }
}

int main() {
    roc_log_set_level(ROC_LOG_INFO);

    /* Run two receivers in separate threads. */
    pthread_t receiver1_thread;
    if (pthread_create(&receiver1_thread, NULL, receiver_loop, (void*)1) != 0) {
        oops();
    }
    pthread_t receiver2_thread;
    if (pthread_create(&receiver2_thread, NULL, receiver_loop, (void*)2) != 0) {
        oops();
    }

    /* Run sender in main thread. */
    sender_loop();

    return 0;
}
