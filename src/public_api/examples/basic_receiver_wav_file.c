/*
 * This example implements minimal receiver that writes to WAV file.
 *
 * Flow:
 *   - creates a receiver and binds it to a local address
 *   - reads audio stream from the receiver and writes it to a WAV file
 *
 * Building:
 *   cc -o basic_receiver_wav_file basic_receiver_wav_file.c -lroc
 *
 * Running:
 *   ./basic_receiver_wav_file
 *
 * License:
 *   public domain
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/log.h>
#include <roc/receiver.h>

/* Network parameters. */
#define MY_RECEIVER_IP "0.0.0.0"
#define MY_RECEIVER_SOURCE_PORT 10101
#define MY_RECEIVER_REPAIR_PORT 10102
#define MY_RECEIVER_CONTROL_PORT 10103

/* Audio parameters. */
#define MY_SAMPLE_RATE 44100
#define MY_CHANNEL_COUNT 2
#define MY_BUFFER_SIZE 1000

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

struct wav_header {
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t format;
    uint32_t subchunk1_id;
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t subchunk2_id;
    uint32_t subchunk2_size;
};

static void
wav_write(FILE* fp, const float* samples, size_t new_samples, size_t total_samples) {
    uint32_t data_len = (uint32_t)total_samples * sizeof(float);

    struct wav_header hdr;
    memset(&hdr, 0, sizeof(hdr));

    /* assume that we're running on little endian cpu */
    memcpy(&hdr.chunk_id, "RIFF", 4);
    hdr.chunk_size = data_len + 36;
    memcpy(&hdr.format, "WAVE", 4);
    memcpy(&hdr.subchunk1_id, "fmt ", 4);
    hdr.subchunk1_size = 16;
    hdr.audio_format = 0x0003; /* WAVE_FORMAT_IEEE_FLOAT */
    hdr.num_channels = MY_CHANNEL_COUNT;
    hdr.sample_rate = MY_SAMPLE_RATE;
    hdr.byte_rate = MY_SAMPLE_RATE * MY_CHANNEL_COUNT * sizeof(float);
    hdr.block_align = MY_CHANNEL_COUNT * sizeof(float);
    hdr.bits_per_sample = 8 * sizeof(float);
    memcpy(&hdr.subchunk2_id, "data", 4);
    hdr.subchunk2_size = data_len;

    /* update header with new sample count */
    fseek(fp, 0, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, fp);

    /* append samples */
    fseek(fp, 0, SEEK_END);
    fwrite(samples, sizeof(float), new_samples, fp);
    fflush(fp);
}

int main() {
    /* Enable more verbose logging. */
    roc_log_set_level(ROC_LOG_INFO);

    /* Initialize context config.
     * Initialize to zero to use default values for all fields. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* Create context.
     * Context contains memory pools and worker thread(s).
     * We need a context to create a receiver. */
    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Initialize receiver config.
     * We keep most fields zero to use default values. */
    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    /* Setup frame format that we want to read from receiver. */
    receiver_config.frame_encoding.format = ROC_FORMAT_PCM;
    receiver_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
    receiver_config.frame_encoding.rate = MY_SAMPLE_RATE;
    receiver_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

    /* Turn on internal CPU timer.
     * Receiver must read packets with steady rate, so we should either implement
     * clocking or ask the library to do so. We choose the second here. */
    receiver_config.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

    /* Create receiver. */
    roc_receiver* receiver = NULL;
    if (roc_receiver_open(context, &receiver_config, &receiver) != 0) {
        oops();
    }

    /* Bind receiver to the source (audio) packets endpoint.
     * The receiver will expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on this port. */
    roc_endpoint* source_endp = NULL;
    if (roc_endpoint_allocate(&source_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(source_endp, ROC_PROTO_RTP_RS8M_SOURCE);
    roc_endpoint_set_host(source_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(source_endp, MY_RECEIVER_SOURCE_PORT);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_SOURCE,
                          source_endp)
        != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(source_endp) != 0) {
        oops();
    }

    /* Bind receiver to the repair (FEC) packets endpoint.
     * The receiver will expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on this port. */
    roc_endpoint* repair_endp = NULL;
    if (roc_endpoint_allocate(&repair_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(repair_endp, ROC_PROTO_RS8M_REPAIR);
    roc_endpoint_set_host(repair_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(repair_endp, MY_RECEIVER_REPAIR_PORT);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_REPAIR,
                          repair_endp)
        != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(repair_endp) != 0) {
        oops();
    }

    /* Bind receiver to the control (RTCP) packets endpoint. */
    roc_endpoint* control_endp = NULL;
    if (roc_endpoint_allocate(&control_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(control_endp, ROC_PROTO_RTCP);
    roc_endpoint_set_host(control_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(control_endp, MY_RECEIVER_CONTROL_PORT);

    if (roc_receiver_bind(receiver, ROC_SLOT_DEFAULT, ROC_INTERFACE_AUDIO_CONTROL,
                          control_endp)
        != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(control_endp) != 0) {
        oops();
    }

    /* Open output file. */
    size_t total_samples = 0;
    FILE* wav_file = fopen("receiver_output.wav", "w");
    if (!wav_file) {
        oops();
    }

    /* Receive and play samples. */
    for (;;) {
        /* Read samples from receiver.
         * If not enough samples are received, receiver will pad buffer with zeros. */
        float samples[MY_BUFFER_SIZE];

        roc_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_receiver_read(receiver, &frame) != 0) {
            oops();
        }

        /* Write samples to file. */
        total_samples += MY_BUFFER_SIZE;
        wav_write(wav_file, samples, MY_BUFFER_SIZE, total_samples);
    }

    /* Close file. */
    fclose(wav_file);

    /* Destroy receiver. */
    if (roc_receiver_close(receiver) != 0) {
        oops();
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
