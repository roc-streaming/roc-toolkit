/*
 * Register custom Packet loss concealment (PLC) plugin.
 *
 * PLC allows to reduce distortion caused by packet losses by replacing
 * gaps with interpolated data. It is used only when FEC wasn't able to
 * repair lost packets.
 *
 * Building:
 *   cc -o plugin_plc plugin_plc.c -lroc
 *
 * Running:
 *   ./plugin_plc
 *
 * License:
 *   public domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/log.h>
#include <roc/receiver.h>

/* Any number in range [ROC_PLUGIN_ID_MIN; ROC_PLUGIN_ID_MAX] */
#define MY_PLC_PLUGIN_ID ROC_PLUGIN_ID_MIN + 1

/* Audio parameters. */
#define MY_SAMPLE_RATE 44100
#define MY_CHANNEL_COUNT 2

/* How much sample after a gap PLC needs for interpolation. */
#define MY_LOOKAHEAD_SIZE 4410 /* 100 ms */

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

/* PLC plugin instance.
 * roc_receiver will create an instance for every connection. */
struct my_plc {
    /* Here we could put state needed for interpolation. */
    unsigned int history_frame_counter;
    unsigned int lost_frame_counter;
};

/* Create plugin instance. */
static void* my_plc_new(roc_plugin_plc* plugin) {
    return calloc(1, sizeof(struct my_plc));
}

/* Delete plugin instance. */
static void my_plc_delete(void* plugin_instance) {
    struct my_plc* plc = (struct my_plc*)plugin_instance;

    free(plc);
}

/* Get look-ahead length - how many samples after the lost frame
 * do we need for interpolation.
 * Returned value is measured as the number of samples per channel,
 * e.g. if sample rate is 44100Hz, length 4410 is 100ms */
static unsigned int my_plc_lookahead_len(void* plugin_instance) {
    return MY_LOOKAHEAD_SIZE;
}

/* Called when next frame is good (no loss). */
static void my_plc_process_history(void* plugin_instance,
                                   const roc_frame* history_frame) {
    struct my_plc* plc = (struct my_plc*)plugin_instance;

    /* Here we can copy samples from history_frame to ring buffer.
     * In this example we just ignore frame. */
    plc->history_frame_counter++;
}

/* Called when next frame is lost and we must fill it with interpolated data.
 *
 * lost_frame is the frame to be filled (we must fill its buffer with the
 * interpolated samples)
 *
 * lookahead_frame contains samples going after the lost frame, which we can
 * use to improve interpolation results. Its size may vary from 0 to MY_LOOKAHEAD_SIZE.
 */
static void my_plc_process_loss(void* plugin_instance,
                                roc_frame* lost_frame,
                                const roc_frame* lookahead_frame) {
    struct my_plc* plc = (struct my_plc*)plugin_instance;

    /* Here we can implement interpolation.
     * In this example we just fill frame with constants.
     * Samples are float because we use ROC_FORMAT_PCM_FLOAT32.
     * There are two channels because we use ROC_CHANNEL_LAYOUT_STEREO. */
    float* lost_samples = lost_frame->samples;
    size_t lost_sample_count =
        lost_frame->samples_size / sizeof(float) / MY_CHANNEL_COUNT;

    for (unsigned ns = 0; ns < lost_sample_count; ns++) {
        for (unsigned c = 0; c < MY_CHANNEL_COUNT; c++) {
            lost_samples[0] = 0.123f; /* left channel */
            lost_samples[1] = 0.456f; /* right channel */
        }
        lost_samples += MY_CHANNEL_COUNT;
    }

    plc->lost_frame_counter++;
}

int main() {
    roc_log_set_level(ROC_LOG_INFO);

    /* Create context. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Register plugin. */
    roc_plugin_plc plc_plugin;
    memset(&plc_plugin, 0, sizeof(plc_plugin));

    plc_plugin.new_cb = &my_plc_new;
    plc_plugin.delete_cb = &my_plc_delete;
    plc_plugin.lookahead_len_cb = &my_plc_lookahead_len;
    plc_plugin.process_history_cb = &my_plc_process_history;
    plc_plugin.process_loss_cb = &my_plc_process_loss;

    if (roc_context_register_plc(context, MY_PLC_PLUGIN_ID, &plc_plugin) != 0) {
        oops();
    }

    /* Prepare receiver config. */
    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    /* Setup frame format.
     * This format applies to frames that we read from receiver, as well as to
     * the frames passed to PLC plugin. */
    receiver_config.frame_encoding.rate = MY_SAMPLE_RATE;
    receiver_config.frame_encoding.format = ROC_FORMAT_PCM_FLOAT32;
    receiver_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

    /* Enable PLC plugin. */
    receiver_config.plc_backend = (roc_plc_backend)MY_PLC_PLUGIN_ID;

    /* Create receiver. */
    roc_receiver* receiver = NULL;
    if (roc_receiver_open(context, &receiver_config, &receiver) != 0) {
        oops();
    }

    /*
     * Here we can run receiver loop.
     */

    /* Destroy receiver. */
    if (roc_receiver_close(receiver) != 0) {
        oops();
    }

    /* Destroy context.
     * Note that registered plugin must remain valid until this point. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
