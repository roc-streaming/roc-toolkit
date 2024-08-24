/*
 * This example implements minimal sender that generates a sine wave.
 *
 * Flow:
 *   - creates a sender and connects it to remote address
 *   - generates a 10-second beep and writes it to the sender
 *
 * Building:
 *   cc -o basic_sender_sine_wave basic_sender_sine_wave.c -lroc -lm
 *
 * Running:
 *   ./basic_sender_sine_wave
 *
 * License:
 *   public domain
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/log.h>
#include <roc/sender.h>

/* Network parameters. */
#define MY_RECEIVER_IP "127.0.0.1"
#define MY_RECEIVER_SOURCE_PORT 10101
#define MY_RECEIVER_REPAIR_PORT 10102
#define MY_RECEIVER_CONTROL_PORT 10103

/* Audio parameters. */
#define MY_SAMPLE_RATE 44100
#define MY_SINE_RATE 440
#define MY_SINE_DURATION (MY_SAMPLE_RATE * 10)
#define MY_BUFFER_SIZE 100

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

static void generate_sine(float* samples, size_t batch_num, size_t num_samples) {
    double t = batch_num * num_samples / 2.0;
    size_t i;
    for (i = 0; i < num_samples / 2; i++) {
        const float s =
            (float)sin(2 * 3.14159265359 * MY_SINE_RATE / MY_SAMPLE_RATE * t) * 0.1f;

        /* Fill samples for left and right channels. */
        samples[i * 2] = s;
        samples[i * 2 + 1] = -s;

        t += 1;
    }
}

int main() {
    /* Enable more verbose logging. */
    roc_log_set_level(ROC_LOG_INFO);

    /* Initialize context config.
     * Initialize to zero to use default values for all fields. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* Create context.
     * Context contains memory pools and the worker thread(s).
     * We need a context to create a sender. */
    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Initialize sender config.
     * We keep most fields zero to use default values. */
    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    /* Setup frame format that we want to write to sender. */
    sender_config.frame_encoding.format = ROC_FORMAT_PCM;
    sender_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
    sender_config.frame_encoding.rate = MY_SAMPLE_RATE;
    sender_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

    /* Setup network packets format that sender should generate. */
    sender_config.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;

    /* Turn on internal CPU timer.
     * Sender must send packets with steady rate, so we should either implement
     * clocking or ask the library to do so. We choose the second here. */
    sender_config.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

    /* Create sender. */
    roc_sender* sender = NULL;
    if (roc_sender_open(context, &sender_config, &sender) != 0) {
        oops();
    }

    /* Connect sender to the receiver source (audio) packets endpoint.
     * The receiver should expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on that port. */
    roc_endpoint* source_endp = NULL;
    if (roc_endpoint_allocate(&source_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(source_endp, ROC_PROTO_RTP_RS8M_SOURCE);
    roc_endpoint_set_host(source_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(source_endp, MY_RECEIVER_SOURCE_PORT);

    if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                           source_endp)
        != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(source_endp) != 0) {
        oops();
    }

    /* Connect sender to the receiver repair (FEC) packets endpoint.
     * The receiver should expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on that port. */
    roc_endpoint* repair_endp = NULL;
    if (roc_endpoint_allocate(&repair_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(repair_endp, ROC_PROTO_RS8M_REPAIR);
    roc_endpoint_set_host(repair_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(repair_endp, MY_RECEIVER_REPAIR_PORT);

    if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_REPAIR,
                           repair_endp)
        != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(repair_endp) != 0) {
        oops();
    }

    /* Connect sender to the receiver control (RTCP) packets endpoint. */
    roc_endpoint* control_endp = NULL;
    if (roc_endpoint_allocate(&control_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(control_endp, ROC_PROTO_RTCP);
    roc_endpoint_set_host(control_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(control_endp, MY_RECEIVER_CONTROL_PORT);

    if (roc_sender_connect(sender, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_CONTROL,
                           control_endp)
        != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(control_endp) != 0) {
        oops();
    }

    /* Generate sine wave and write it to the sender. */
    size_t i;
    for (i = 0; i < MY_SINE_DURATION / MY_BUFFER_SIZE; i++) {
        /* Generate sine wave. */
        float samples[MY_BUFFER_SIZE];
        generate_sine(samples, i, MY_BUFFER_SIZE);

        /* Write samples to the sender. */
        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_sender_write(sender, &frame) != 0) {
            oops();
        }
    }

    /* Destroy sender. */
    if (roc_sender_close(sender) != 0) {
        oops();
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
