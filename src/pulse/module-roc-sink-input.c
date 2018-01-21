/*
 * This file is part of Roc PulseAudio integration.
 *
 * Copyright (c) 2017 Roc authors
 *
 * Licensed under GNU Lesser General Public License 2.1 or any later version.
 */

/* config.h from pulseaudio directory (generated after ./configure) */
#include <config.h>

/* public pulseaudio headers */
#include <pulse/xmalloc.h>

/* private pulseaudio headers */
#include <pulsecore/log.h>
#include <pulsecore/modargs.h>
#include <pulsecore/module.h>
#include <pulsecore/namereg.h>
#include <pulsecore/sink-input.h>

/* roc headers */
#include <roc/context.h>
#include <roc/log.h>
#include <roc/receiver.h>

/* local headers */
#include "roc_helpers.h"

PA_MODULE_AUTHOR("Roc authors");
PA_MODULE_DESCRIPTION("Read samples using Roc receiver");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(false);
PA_MODULE_USAGE(
        "sink=<name for the sink> "
        "sink_input_properties=<properties for the sink input> "
        "local_ip=<local receiver ip> "
        "local_source_port=<local receiver port for source packets> "
        "local_repair_port=<local receiver port for repair packets>");

struct roc_sink_input_userdata {
    pa_module* module;
    pa_sink_input* sink_input;

    roc_context* context;
    roc_receiver* receiver;
};

static const char* const roc_sink_input_modargs[] = {
    "sink",
    "sink_input_name",
    "sink_input_properties",
    "local_ip",
    "local_source_port",
    "local_repair_port",
    NULL
};

static int process_message(
    pa_msgobject* o, int code, void* data, int64_t offset, pa_memchunk* chunk) {
    struct roc_sink_input_userdata *u = PA_SINK_INPUT(o)->userdata;
    pa_assert(u);

    switch (code) {
    case PA_SINK_INPUT_MESSAGE_GET_LATENCY:
        /* TODO: we should report internal latency here */
        *((pa_usec_t*)data) = 0;

        /* don't return, the default handler will add in the extra latency
         * added by the resampler
         */
        break;
    }

    return pa_sink_input_process_msg(o, code, data, offset, chunk);
}

static int pop_cb(pa_sink_input* i, size_t length, pa_memchunk* chunk) {
    pa_sink_input_assert_ref(i);

    struct roc_sink_input_userdata* u = i->userdata;
    pa_assert(u);

    /* ensure that all chunk fields are set to zero */
    pa_memchunk_reset(chunk);

    /* allocate memblock */
    chunk->memblock = pa_memblock_new(u->module->core->mempool, length);

    /* start writing memblock */
    char *buf = pa_memblock_acquire(chunk->memblock);

    /* prepare audio frame */
    roc_frame frame;
    memset(&frame, 0, sizeof(frame));

    frame.samples = (float*)buf;
    frame.num_samples = length / pa_sample_size(&u->sink_input->sample_spec);

    /* read samples from file to memblock */
    int ret = roc_receiver_read(u->receiver, &frame);

    /* finish writing memblock */
    pa_memblock_release(chunk->memblock);

    /* handle eof and error */
    if (ret != 0) {
        pa_module_unload_request(u->module, true);
        return -1;
    }

    /* setup chunk boundaries */
    chunk->index = 0;
    chunk->length = (size_t)ret * pa_sample_size(&u->sink_input->sample_spec);

    return 0;
}

static void rewind_cb(pa_sink_input* i, size_t nbytes) {
    pa_sink_input_assert_ref(i);

    struct roc_sink_input_userdata* u = i->userdata;
    pa_assert(u);

    (void)nbytes;
}

static void kill_cb(pa_sink_input* i) {
    pa_sink_input_assert_ref(i);

    struct roc_sink_input_userdata* u = i->userdata;
    pa_assert(u);

    pa_module_unload_request(u->module, true);

    pa_sink_input_unlink(u->sink_input);
    pa_sink_input_unref(u->sink_input);
    u->sink_input = NULL;
}

void pa__done(pa_module*);

int pa__init(pa_module* m) {
    pa_assert(m);

    /* setup logs */
    roc_log_set_level(ROC_LOG_DEBUG);
    roc_log_set_handler(log_handler);

    /* prepare sample spec and channel map used in this sink */
    pa_sample_spec sample_spec;
    sample_spec.format = PA_SAMPLE_FLOAT32LE;
    sample_spec.rate = 44100;
    sample_spec.channels = 2;

    pa_channel_map channel_map;
    pa_channel_map_init_stereo(&channel_map);

    /* get module arguments (key-value list passed to load-module) */
    pa_modargs *args;
    if (!(args = pa_modargs_new(m->argument, roc_sink_input_modargs))) {
        pa_log("failed to parse module arguments");
        goto error;
    }

    /* get sink from arguments */
    pa_sink *sink = pa_namereg_get(
        m->core, pa_modargs_get_value(args, "sink", NULL), PA_NAMEREG_SINK);
    if (!sink) {
        pa_log("sink does not exist");
        goto error;
    }

    /* create and initialize module-specific data */
    struct roc_sink_input_userdata *u =
        pa_xnew0(struct roc_sink_input_userdata, 1);
    pa_assert(u);
    m->userdata = u;

    u->module = m;

    roc_address local_source_addr;
    if (parse_address(&local_source_addr, args, "local_ip", DEFAULT_IP,
                      "local_source_port", DEFAULT_SOURCE_PORT)
        < 0) {
        goto error;
    }

    roc_address local_repair_addr;
    if (parse_address(&local_repair_addr, args, "local_ip", DEFAULT_IP,
                      "local_repair_port", DEFAULT_REPAIR_PORT)
        < 0) {
        goto error;
    }

    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    u->context = roc_context_open(&context_config);
    if (!u->context) {
        pa_log("can't create roc context");
        goto error;
    }

    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    u->receiver = roc_receiver_open(u->context, &receiver_config);
    if (!u->receiver) {
        pa_log("can't create roc receiver");
        goto error;
    }

    if (roc_receiver_bind(u->receiver, ROC_PROTO_RTP_RSM8_SOURCE, &local_source_addr)
        != 0) {
        pa_log("can't connect roc receiver to local address");
        goto error;
    }

    if (roc_receiver_bind(u->receiver, ROC_PROTO_RSM8_REPAIR, &local_repair_addr) != 0) {
        pa_log("can't connect roc receiver to local address");
        goto error;
    }

    if (roc_context_start(u->context) != 0) {
        pa_log("can't start roc receiver");
        goto error;
    }

    /* create and initialize sink input */
    pa_sink_input_new_data data;
    pa_sink_input_new_data_init(&data);
    pa_sink_input_new_data_set_sink(&data, sink, false);
    data.driver = "roc_receiver";
    data.module = u->module;
    pa_sink_input_new_data_set_sample_spec(&data, &sample_spec);
    pa_sink_input_new_data_set_channel_map(&data, &channel_map);

    pa_proplist_sets(data.proplist, PA_PROP_MEDIA_NAME, "Roc Receiver");

    if (pa_modargs_get_proplist(
            args,
            "sink_input_properties",
            data.proplist,
            PA_UPDATE_REPLACE) < 0) {
        pa_log("invalid sink input properties");
        pa_sink_input_new_data_done(&data);
        goto error;
    }

    pa_sink_input_new(&u->sink_input, u->module->core, &data);
    pa_sink_input_new_data_done(&data);

    if (!u->sink_input) {
        pa_log("failed to create sink input");
        goto error;
    }

    u->sink_input->userdata = u;
    u->sink_input->parent.process_msg = process_message;
    u->sink_input->pop = pop_cb;
    u->sink_input->process_rewind = rewind_cb;
    u->sink_input->kill = kill_cb;

    pa_sink_input_put(u->sink_input);
    pa_modargs_free(args);

    return 0;

error:
    if (args) {
        pa_modargs_free(args);
    }
    pa__done(m);

    return -1;
}

void pa__done(pa_module* m) {
    pa_assert(m);

    struct roc_sink_input_userdata *u = m->userdata;
    if (!u) {
        return;
    }

    if (u->sink_input) {
        pa_sink_input_unlink(u->sink_input);
        pa_sink_input_unref(u->sink_input);
    }

    if (u->receiver) {
        roc_receiver_close(u->receiver);
    }

    if (u->context) {
        roc_context_stop(u->context);
        roc_context_close(u->context);
    }

    pa_xfree(u);
}
