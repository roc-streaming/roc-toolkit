/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string.h>

#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"
#include "roc_sndio/pulseaudio_sink.h"

namespace roc {
namespace sndio {

namespace {

const core::nanoseconds_t ReportInterval = 10 * core::Second;

} // namespace

PulseaudioSink::PulseaudioSink(const Config& config)
    : device_(NULL)
    , sample_rate_(config.sample_rate)
    , num_channels_(packet::num_channels(config.channels))
    , open_done_(false)
    , opened_(false)
    , broken_(false)
    , mainloop_(NULL)
    , context_(NULL)
    , sink_info_op_(NULL)
    , stream_(NULL)
    , rate_limiter_(ReportInterval) {
    if (config.latency != 0) {
        latency_ = config.latency;
    } else {
        latency_ = 40 * core::Millisecond;
    }
}

PulseaudioSink::~PulseaudioSink() {
    close_();
}

bool PulseaudioSink::open(const char* device) {
    if (mainloop_) {
        roc_panic("pulseaudio sink: can't call open() twice");
    }

    roc_log(LogDebug, "pulseaudio sink: opening sink: device=%s", device);

    if (!check_params_()) {
        return false;
    }

    if (!start_()) {
        return false;
    }

    if (!open_(device)) {
        return false;
    }

    return true;
}

size_t PulseaudioSink::sample_rate() const {
    check_started_();

    pa_threaded_mainloop_lock(mainloop_);

    check_opened_();

    const size_t ret = sample_rate_;

    pa_threaded_mainloop_unlock(mainloop_);

    return ret;
}

bool PulseaudioSink::has_clock() const {
    return true;
}

void PulseaudioSink::write(audio::Frame& frame) {
    check_started_();

    const audio::sample_t* data = frame.data();
    size_t size = frame.size();

    while (size > 0) {
        pa_threaded_mainloop_lock(mainloop_);

        const ssize_t ret = write_stream_(data, size);
        if (ret < 0) {
            set_broken_();
        }

        pa_threaded_mainloop_unlock(mainloop_);

        if (ret < 0) {
            break;
        }

        data += (size_t)ret;
        size -= (size_t)ret;
    }
}

void PulseaudioSink::close_() {
    if (!mainloop_) {
        return;
    }

    roc_log(LogDebug, "pulseaudio sink: closing sink");

    pa_threaded_mainloop_lock(mainloop_);

    close_stream_();
    close_sink_info_op_();
    close_context_();

    pa_threaded_mainloop_unlock(mainloop_);

    pa_threaded_mainloop_stop(mainloop_);
    pa_threaded_mainloop_free(mainloop_);

    mainloop_ = NULL;
}

bool PulseaudioSink::check_params_() const {
    if (num_channels_ == 0) {
        roc_log(LogError, "pulseaudio sink: # of channels is zero");
        return false;
    }

    if (latency_ <= 0) {
        roc_log(LogError, "pulseaudio sink: latency should be positive");
        return false;
    }

    return true;
}

void PulseaudioSink::check_started_() const {
    if (!mainloop_) {
        roc_panic("pulseaudio sink: can't use unopened sink");
    }
}

void PulseaudioSink::check_opened_() const {
    if (!opened_) {
        roc_panic("pulseaudio sink: can't use unopened sink");
    }
}

void PulseaudioSink::set_opened_(bool opened) {
    if (opened) {
        roc_log(LogTrace, "pulseaudio sink: successfully opened sink");
    } else {
        roc_log(LogError, "pulseaudio sink: failed to open sink");
    }

    open_done_ = true;
    opened_ = opened;

    pa_threaded_mainloop_signal(mainloop_, 0);
}

void PulseaudioSink::set_broken_() {
    if (!broken_) {
        roc_log(LogTrace, "pulseaudio sink: stream is broken");
    }

    broken_ = true;
}

bool PulseaudioSink::start_() {
    mainloop_ = pa_threaded_mainloop_new();
    if (!mainloop_) {
        roc_log(LogError, "pulseaudio sink: pa_threaded_mainloop_new() failed");
        return false;
    }

    if (int err = pa_threaded_mainloop_start(mainloop_)) {
        roc_log(LogError, "pulseaudio sink: pa_threaded_mainloop_start(): %s",
                pa_strerror(err));
        return false;
    }

    return true;
}

bool PulseaudioSink::open_(const char* device) {
    pa_threaded_mainloop_lock(mainloop_);

    if (device && strcmp(device, "default") != 0) {
        device_ = device;
    }

    if (open_context_()) {
        while (!open_done_) {
            pa_threaded_mainloop_wait(mainloop_);
        }
    }

    const bool ret = opened_;

    pa_threaded_mainloop_unlock(mainloop_);

    return ret;
}

bool PulseaudioSink::open_context_() {
    roc_log(LogTrace, "pulseaudio sink: opening context");

    context_ = pa_context_new(pa_threaded_mainloop_get_api(mainloop_), "Roc");
    if (!context_) {
        roc_log(LogError, "pulseaudio sink: pa_context_new() failed");
        return false;
    }

    pa_context_set_state_callback(context_, context_state_cb_, this);

    if (int err = pa_context_connect(context_, NULL, PA_CONTEXT_NOFLAGS, NULL)) {
        roc_log(LogError, "pulseaudio sink: pa_context_connect(): %s", pa_strerror(err));
        return false;
    }

    return true;
}

void PulseaudioSink::close_context_() {
    if (!context_) {
        return;
    }

    roc_log(LogTrace, "pulseaudio sink: closing context");

    pa_context_disconnect(context_);
    pa_context_unref(context_);

    context_ = NULL;
}

void PulseaudioSink::context_state_cb_(pa_context* context, void* userdata) {
    PulseaudioSink& self = *(PulseaudioSink*)userdata;

    if (self.opened_) {
        return;
    }

    const pa_context_state_t state = pa_context_get_state(context);

    switch ((unsigned)state) {
    case PA_CONTEXT_READY:
        roc_log(LogTrace, "pulseaudio sink: successfully opened context");

        if (!self.start_sink_info_op_()) {
            self.set_opened_(false);
        }

        break;

    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        roc_log(LogError, "pulseaudio sink: failed to open context");

        self.set_opened_(false);
        break;

    default:
        break;
    }
}

bool PulseaudioSink::start_sink_info_op_() {
    roc_panic_if(sink_info_op_);

    roc_log(LogTrace, "pulseaudio sink: requesting sink info");

    sink_info_op_ =
        pa_context_get_sink_info_by_name(context_, device_, sink_info_cb_, this);

    return sink_info_op_;
}

void PulseaudioSink::close_sink_info_op_() {
    if (!sink_info_op_) {
        return;
    }

    pa_operation_cancel(sink_info_op_);
    pa_operation_unref(sink_info_op_);

    sink_info_op_ = NULL;
}

void PulseaudioSink::sink_info_cb_(pa_context*,
                                   const pa_sink_info* info,
                                   int,
                                   void* userdata) {
    PulseaudioSink& self = *(PulseaudioSink*)userdata;

    self.close_sink_info_op_();

    if (!info) {
        roc_log(LogError, "pulseaudio sink: failed to retrieve sink info");
        self.set_opened_(false);
        return;
    }

    roc_log(LogTrace, "pulseaudio sink: successfully retrieved sink info");

    self.init_stream_params_(*info);

    if (!self.open_stream_()) {
        self.set_opened_(false);
    }
}

void PulseaudioSink::init_stream_params_(const pa_sink_info& info) {
    if (sample_rate_ == 0) {
        sample_rate_ = (size_t)info.sample_spec.rate;
    }

    sample_spec_.format = PA_SAMPLE_FLOAT32LE;
    sample_spec_.rate = (uint32_t)sample_rate_;
    sample_spec_.channels = (uint8_t)num_channels_;

    const size_t latency = (size_t)packet::timestamp_from_ns(latency_, sample_rate_)
        * num_channels_ * sizeof(audio::sample_t);

    buffer_attrs_.maxlength = (uint32_t)latency;
    buffer_attrs_.tlength = (uint32_t)latency;
    buffer_attrs_.prebuf = (uint32_t)-1;
    buffer_attrs_.minreq = (uint32_t)-1;
    buffer_attrs_.fragsize = (uint32_t)-1;
}

bool PulseaudioSink::open_stream_() {
    roc_panic_if_not(context_);

    roc_log(LogDebug,
            "pulseaudio sink: opening stream: device=%s n_channels=%lu sample_rate=%lu",
            device_, (unsigned long)num_channels_, (unsigned long)sample_rate_);

    stream_ = pa_stream_new(context_, "Roc", &sample_spec_, NULL);
    if (!stream_) {
        roc_log(LogError, "pulseaudio sink: pa_stream_new(): %s",
                pa_strerror(pa_context_errno(context_)));
        return false;
    }

    const pa_stream_flags_t flags =
        pa_stream_flags_t(PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE);

    pa_stream_set_state_callback(stream_, stream_state_cb_, this);
    pa_stream_set_write_callback(stream_, stream_write_cb_, this);
    pa_stream_set_latency_update_callback(stream_, stream_latency_cb_, this);

    int err =
        pa_stream_connect_playback(stream_, device_, &buffer_attrs_, flags, NULL, NULL);

    if (err != 0) {
        roc_log(LogError, "pulseaudio sink: pa_stream_connect_playback(): %s",
                pa_strerror(err));
        return false;
    }

    return true;
}

void PulseaudioSink::close_stream_() {
    if (!stream_) {
        return;
    }

    roc_log(LogTrace, "pulseaudio sink: closing stream");

    pa_stream_disconnect(stream_);
    pa_stream_unref(stream_);

    stream_ = NULL;
}

ssize_t PulseaudioSink::write_stream_(const audio::sample_t* data, size_t size) {
    check_opened_();

    if (broken_) {
        return -1;
    }

    size_t writable_size;

    for (;;) {
        writable_size = pa_stream_writable_size(stream_);

        roc_log(LogTrace, "pulseaudio sink: write: requested_size=%lu writable_size=%lu",
                (unsigned long)size, (unsigned long)writable_size);

        if (writable_size == (size_t)-1) {
            roc_log(LogError, "pulseaudio sink: pa_stream_writable_size() failed");
            return -1;
        }

        if (writable_size != 0) {
            break;
        }

        pa_threaded_mainloop_wait(mainloop_);
    }

    if (size > writable_size) {
        size = writable_size;
    }

    int err = pa_stream_write(stream_, data, size * sizeof(audio::sample_t), NULL, 0,
                              PA_SEEK_RELATIVE);

    if (err != 0) {
        roc_log(LogError, "pulseaudio sink: pa_stream_write(): %s", pa_strerror(err));
        return -1;
    }

    return (ssize_t)size;
}

void PulseaudioSink::stream_state_cb_(pa_stream* stream, void* userdata) {
    PulseaudioSink& self = *(PulseaudioSink*)userdata;

    if (self.opened_) {
        return;
    }

    const pa_stream_state_t state = pa_stream_get_state(stream);

    switch ((unsigned)state) {
    case PA_STREAM_READY:
        roc_log(LogTrace, "pulseaudio sink: successfully opened stream");

        self.set_opened_(true);
        break;

    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
        roc_log(LogError, "pulseaudio sink: failed to open stream");

        self.set_opened_(false);
        break;

    default:
        break;
    }
}

void PulseaudioSink::stream_write_cb_(pa_stream*, size_t length, void* userdata) {
    PulseaudioSink& self = *(PulseaudioSink*)userdata;

    if (length != 0) {
        pa_threaded_mainloop_signal(self.mainloop_, 0);
    }
}

void PulseaudioSink::stream_latency_cb_(pa_stream* stream, void* userdata) {
    PulseaudioSink& self = *(PulseaudioSink*)userdata;

    if (!self.rate_limiter_.allow()) {
        return;
    }

    pa_usec_t latency_us = 0;
    int negative = 0;

    if (int err = pa_stream_get_latency(stream, &latency_us, &negative)) {
        roc_log(LogError, "pulseaudio sink: pa_stream_get_latency(): %s",
                pa_strerror(err));
        return;
    }

    ssize_t latency = (ssize_t)(pa_usec_to_bytes(latency_us, &self.sample_spec_)
                                / sizeof(audio::sample_t) / self.num_channels_);

    if (negative) {
        latency = -latency;
    }

    roc_log(LogDebug, "pulseaudio sink: stream_latency=%ld", (long)latency);
}

} // namespace sndio
} // namespace roc
