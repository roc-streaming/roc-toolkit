/*
 * This example shows how to use slots mechanism to bind receiver to two different
 * addresses (for example on different network interfaces or using different
 * network protocols), and then connect two senders to those address.
 *
 * Flow:
 *   - creates a receiver
 *   - binds slot 1 of the receiver to the first address, using bare RTP
 *   - binds slot 2 of the receiver to the second address, using RTP + FECFRAME + RTCP
 *   - creates two senders and connects each one to its own address of the receiver
 *   - two threads writes audio stream to the senders
 *   - another thread reads mixed audio stream from receiver
 *
 * Building:
 *   cc -o send_recv_2_senders_1_receiver send_recv_2_senders_1_receiver.c -lroc
 *
 * Running:
 *   ./send_recv_2_senders_1_receiver
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

/* First slot has one bare RTP endpoint. */
#define MY_RECEIVER_SLOT_1_SOURCE_ENDPOINT "rtp://127.0.0.1:10201"

/* Second slot has has three endpoints: RTP + FECFRAME + RTCP */
#define MY_RECEIVER_SLOT_2_SOURCE_ENDPOINT "rtp+rs8m://127.0.0.1:10301"
#define MY_RECEIVER_SLOT_2_REPAIR_ENDPOINT "rs8m://127.0.0.1:10302"
#define MY_RECEIVER_SLOT_2_CONTROL_ENDPOINT "rtcp://127.0.0.1:10303"

/* Receiver slot identifiers, can be arbitrary numbers. */
#define MY_RECEIVER_SLOT_1 1
#define MY_RECEIVER_SLOT_2 2

/* Audio parameters. */
#define MY_SAMPLE_RATE 44100
#define MY_BUFFER_SIZE 2000

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

static void receiver_loop() {
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

    /* Bind one RTP endpoint of MY_RECEIVER_SLOT_1.
     *
     * Note that there is no need to explicitly create slot, we just pass slot id
     * to roc_receiver_bind() and the slot is created automatically.
     */
    roc_endpoint* slot_1_source_endp = NULL;
    roc_endpoint_allocate(&slot_1_source_endp);
    roc_endpoint_set_uri(slot_1_source_endp, MY_RECEIVER_SLOT_1_SOURCE_ENDPOINT);

    if (roc_receiver_bind(receiver, MY_RECEIVER_SLOT_1, ROC_INTERFACE_AUDIO_SOURCE,
                          slot_1_source_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_1_source_endp);

    /* Bind three endpoints of MY_RECEIVER_SLOT_2.
     *
     * As you can see, different slots may have different sets of endpoints and
     * use different protocols.
     */
    roc_endpoint* slot_2_source_endp = NULL;
    roc_endpoint_allocate(&slot_2_source_endp);
    roc_endpoint_set_uri(slot_2_source_endp, MY_RECEIVER_SLOT_2_SOURCE_ENDPOINT);

    if (roc_receiver_bind(receiver, MY_RECEIVER_SLOT_2, ROC_INTERFACE_AUDIO_SOURCE,
                          slot_2_source_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_1_source_endp);

    roc_endpoint* slot_2_repair_endp = NULL;
    roc_endpoint_allocate(&slot_2_repair_endp);
    roc_endpoint_set_uri(slot_2_repair_endp, MY_RECEIVER_SLOT_2_REPAIR_ENDPOINT);

    if (roc_receiver_bind(receiver, MY_RECEIVER_SLOT_2, ROC_INTERFACE_AUDIO_REPAIR,
                          slot_2_repair_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_2_repair_endp);

    roc_endpoint* slot_2_control_endp = NULL;
    roc_endpoint_allocate(&slot_2_control_endp);
    roc_endpoint_set_uri(slot_2_control_endp, MY_RECEIVER_SLOT_2_CONTROL_ENDPOINT);

    if (roc_receiver_bind(receiver, MY_RECEIVER_SLOT_2, ROC_INTERFACE_AUDIO_CONTROL,
                          slot_2_control_endp)
        != 0) {
        oops();
    }
    roc_endpoint_deallocate(slot_2_control_endp);

    /* Read samples from the receiver.
     * As there are 2 sender, we'll get mixed stream. */
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

    /* Destroy receiver and context. */
    if (roc_receiver_close(receiver) != 0) {
        oops();
    }
    if (roc_context_close(context) != 0) {
        oops();
    }
}

static void* sender_loop(void* arg) {
    long sender_id = (long)arg;

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

    sender_config.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;

    /* First receiver slot uses bare RTP, second uses FEC. */
    sender_config.fec_encoding =
        sender_id == 1 ? ROC_FEC_ENCODING_DISABLE : ROC_FEC_ENCODING_RS8M;

    /* Make write operation blocking as we don't have our own clock. */
    sender_config.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

    roc_sender* sender = NULL;
    if (roc_sender_open(context, &sender_config, &sender) != 0) {
        oops();
    }

    if (sender_id == 1) {
        /* We're sender 1.
         * Connect one endpoint to receiver's first slot. */
        roc_endpoint* source_endp = NULL;
        roc_endpoint_allocate(&source_endp);
        roc_endpoint_set_uri(source_endp, MY_RECEIVER_SLOT_1_SOURCE_ENDPOINT);

        if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                               source_endp)
            != 0) {
            oops();
        }
        roc_endpoint_deallocate(source_endp);
    } else {
        /* We're sender 2.
         * Connect three endpoints to receiver's second slot. */
        roc_endpoint* source_endp = NULL;
        roc_endpoint_allocate(&source_endp);
        roc_endpoint_set_uri(source_endp, MY_RECEIVER_SLOT_2_SOURCE_ENDPOINT);

        if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                               source_endp)
            != 0) {
            oops();
        }
        roc_endpoint_deallocate(source_endp);

        roc_endpoint* repair_endp = NULL;
        roc_endpoint_allocate(&repair_endp);
        roc_endpoint_set_uri(repair_endp, MY_RECEIVER_SLOT_2_REPAIR_ENDPOINT);

        if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_REPAIR,
                               repair_endp)
            != 0) {
            oops();
        }
        roc_endpoint_deallocate(repair_endp);

        roc_endpoint* control_endp = NULL;
        roc_endpoint_allocate(&control_endp);
        roc_endpoint_set_uri(control_endp, MY_RECEIVER_SLOT_2_CONTROL_ENDPOINT);

        if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_CONTROL,
                               control_endp)
            != 0) {
            oops();
        }
        roc_endpoint_deallocate(control_endp);
    }

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
            printf(">>> sender %ld frame counter: %lu\n", sender_id, nf);
        }
    }

    /* Destroy sender and context. */
    if (roc_sender_close(sender) != 0) {
        oops();
    }
    if (roc_context_close(context) != 0) {
        oops();
    }

    return NULL;
}

int main() {
    roc_log_set_level(ROC_LOG_INFO);

    /* Run two senders in separate threads. */
    pthread_t sender1_thread;
    if (pthread_create(&sender1_thread, NULL, sender_loop, (void*)1) != 0) {
        oops();
    }
    pthread_t sender2_thread;
    if (pthread_create(&sender2_thread, NULL, sender_loop, (void*)2) != 0) {
        oops();
    }

    /* Run receiver in main thread. */
    receiver_loop();

    return 0;
}
