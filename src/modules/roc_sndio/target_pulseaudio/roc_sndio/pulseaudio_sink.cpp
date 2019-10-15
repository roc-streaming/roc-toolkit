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

const core::nanoseconds_t DefaultLatency = core::Millisecond * 60;

const core::nanoseconds_t MinTimeout = core::Millisecond * 50;
const core::nanoseconds_t MaxTimeout = core::Second * 2;

} // namespace

PulseaudioSink::PulseaudioSink(const Config& config)
    : device_(NULL)
    , sample_rate_(config.sample_rate)
    , num_channels_(packet::num_channels(config.channels))
    , frame_size_(
          packet::ns_to_size(config.frame_length, config.sample_rate, config.channels))
    , open_done_(false)
    , opened_(false)
    , mainloop_(NULL)
    , context_(NULL)
    , sink_info_op_(NULL)
    , stream_(NULL)
    , timer_(NULL)
    , timer_deadline_(0)
    , rate_limiter_(ReportInterval) {
    if (config.latency != 0) {
        latency_ = config.latency;
    } else {
        latency_ = DefaultLatency;
    }
    timeout_ = latency_ * 2;
    if (timeout_ < MinTimeout) {
        timeout_ = MinTimeout;
    }
}

PulseaudioSink::~PulseaudioSink() {
    roc_log(LogInfo, "pulseaudio sink: closing sink");

    close_();
    stop_mainloop_();
}

bool PulseaudioSink::open(const char* device) {
    if (mainloop_) {
        roc_panic("pulseaudio sink: can't call open() twice");
    }

    roc_log(LogDebug, "pulseaudio sink: opening sink: device=%s", device);

    if (device && strcmp(device, "default") != 0) {
        device_ = device;
    }

    if (!check_params_()) {
        return false;
    }

    if (!start_mainloop_()) {
        return false;
    }

    if (!open_()) {
        return false;
    }

    return true;
}

size_t PulseaudioSink::sample_rate() const {
    ensure_started_();

    pa_threaded_mainloop_lock(mainloop_);

    ensure_opened_();

    const size_t ret = sample_rate_;

    pa_threaded_mainloop_unlock(mainloop_);

    return ret;
}

size_t PulseaudioSink::num_channels() const {
    return num_channels_;
}

bool PulseaudioSink::has_clock() const {
    return true;
}

void PulseaudioSink::write(audio::Frame& frame) {
    ensure_started_();

    if (!write_frame_(frame)) {
        roc_log(LogInfo, "pulseaudio sink: restarting stream");

        close_();

        if (!open_()) {
            roc_panic("pulseaudio sink: can't restart stream");
        }
    }
}

bool PulseaudioSink::write_frame_(audio::Frame& frame) {
    const audio::sample_t* data = frame.data();
    size_t size = frame.size();

    while (size > 0) {
        pa_threaded_mainloop_lock(mainloop_);

        const ssize_t ret = write_stream_(data, size);

        pa_threaded_mainloop_unlock(mainloop_);

        if (ret < 0) {
            return false;
        }

        data += (size_t)ret;
        size -= (size_t)ret;
    }

    return true;
}

bool PulseaudioSink::check_params_() const {
    if (num_channels_ == 0) {
        roc_log(LogError, "pulseaudio sink: # of channels is zero");
        return false;
    }

    if (frame_size_ == 0) {
        roc_log(LogError, "pulseaudio sink: frame size is zero");
        return false;
    }

    if (latency_ <= 0) {
        roc_log(LogError, "pulseaudio sink: latency should be positive");
        return false;
    }

    return true;
}

void PulseaudioSink::ensure_started_() const {
    if (!mainloop_) {
        roc_panic("pulseaudio sink: can't use unopened sink");
    }
}

void PulseaudioSink::ensure_opened_() const {
    if (!opened_) {
        roc_panic("pulseaudio sink: can't use unopened sink");
    }
}

bool PulseaudioSink::start_mainloop_() {
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

void PulseaudioSink::stop_mainloop_() {
    if (!mainloop_) {
        return;
    }

    pa_threaded_mainloop_stop(mainloop_);
    pa_threaded_mainloop_free(mainloop_);

    mainloop_ = NULL;
}

bool PulseaudioSink::open_() {
    pa_threaded_mainloop_lock(mainloop_);

    if (open_context_()) {
        while (!open_done_) {
            pa_threaded_mainloop_wait(mainloop_);
        }
    }

    const bool ret = opened_;

    pa_threaded_mainloop_unlock(mainloop_);

    return ret;
}

void PulseaudioSink::close_() {
    if (!mainloop_) {
        return;
    }

    pa_threaded_mainloop_lock(mainloop_);

    stop_timer_();
    close_stream_();
    cancel_sink_info_op_();
    close_context_();

    open_done_ = false;
    opened_ = false;

    pa_threaded_mainloop_unlock(mainloop_);
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
    roc_log(LogTrace, "pulseaudio sink: context state callback");

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
        roc_log(LogTrace, "pulseaudio sink: ignoring unknown context state");
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

void PulseaudioSink::cancel_sink_info_op_() {
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

    self.cancel_sink_info_op_();

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

    roc_panic_if(sizeof(audio::sample_t) != sizeof(float));

    sample_spec_.format = PA_SAMPLE_FLOAT32LE;
    sample_spec_.rate = (uint32_t)sample_rate_;
    sample_spec_.channels = (uint8_t)num_channels_;

    const size_t latency = (size_t)packet::timestamp_from_ns(latency_, sample_rate_)
        * num_channels_ * sizeof(audio::sample_t);

    const size_t frame_size = frame_size_ * sizeof(audio::sample_t);

    buffer_attrs_.maxlength = (uint32_t)-1;
    buffer_attrs_.tlength = (uint32_t)latency;
    buffer_attrs_.prebuf = (uint32_t)-1;
    buffer_attrs_.minreq = (uint32_t)frame_size;
    buffer_attrs_.fragsize = (uint32_t)-1;
}

bool PulseaudioSink::open_stream_() {
    roc_panic_if_not(context_);

    roc_log(LogInfo,
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
    ensure_opened_();

    ssize_t writable_size = wait_stream_();

    if (writable_size == -1) {
        return -1;
    }

    roc_log(LogTrace, "pulseaudio sink: write: requested_size=%lu writable_size=%lu",
            (unsigned long)size, (unsigned long)writable_size);

    if (size > (size_t)writable_size) {
        size = (size_t)writable_size;
    }

    int err = pa_stream_write(stream_, data, size * sizeof(audio::sample_t), NULL, 0,
                              PA_SEEK_RELATIVE);

    if (err != 0) {
        roc_log(LogError, "pulseaudio sink: pa_stream_write(): %s", pa_strerror(err));
        return -1;
    }

    return (ssize_t)size;
}

ssize_t PulseaudioSink::wait_stream_() {
    bool timer_expired = false;

    for (;;) {
        size_t writable_size = pa_stream_writable_size(stream_);

        if (writable_size == (size_t)-1) {
            roc_log(LogError, "pulseaudio sink: stream is broken");
            return -1;
        }

        if (writable_size == 0 && timer_expired) {
            roc_log(LogInfo,
                    "pulseaudio sink: stream timeout expired: latency=%ld timeout=%ld",
                    (long)packet::timestamp_from_ns(latency_, sample_rate_),
                    (long)packet::timestamp_from_ns(timeout_, sample_rate_));

            if (timeout_ < MaxTimeout) {
                timeout_ *= 2;
                if (timeout_ > MaxTimeout) {
                    timeout_ = MaxTimeout;
                }
                roc_log(LogDebug,
                        "pulseaudio sink: stream timeout increased: "
                        "latency=%ld timeout=%ld",
                        (long)packet::timestamp_from_ns(latency_, sample_rate_),
                        (long)packet::timestamp_from_ns(timeout_, sample_rate_));
            }

            return -1;
        }

        if (writable_size != 0) {
            return (ssize_t)writable_size;
        }

        start_timer_(timeout_);

        pa_threaded_mainloop_wait(mainloop_);

        timer_expired = stop_timer_();
    }
}

void PulseaudioSink::stream_state_cb_(pa_stream* stream, void* userdata) {
    roc_log(LogTrace, "pulseaudio sink: stream state callback");

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
        roc_log(LogTrace, "pulseaudio sink: ignoring unknown stream state");
        break;
    }
}

void PulseaudioSink::stream_write_cb_(pa_stream*, size_t length, void* userdata) {
    roc_log(LogTrace, "pulseaudio sink: stream write callback");

    PulseaudioSink& self = *(PulseaudioSink*)userdata;

    if (length != 0) {
        pa_threaded_mainloop_signal(self.mainloop_, 0);
    }
}

void PulseaudioSink::stream_latency_cb_(pa_stream* stream, void* userdata) {
    roc_log(LogTrace, "pulseaudio sink: stream latency callback");

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

void PulseaudioSink::start_timer_(core::nanoseconds_t timeout) {
    roc_panic_if_not(context_);

    const core::nanoseconds_t timeout_usec =
        (timeout + core::Microsecond - 1) / core::Microsecond;

    timer_deadline_ = core::timestamp() + timeout_usec * core::Microsecond;

    const pa_usec_t pa_deadline = pa_rtclock_now() + (pa_usec_t)timeout_usec;

    if (!timer_) {
        timer_ = pa_context_rttime_new(context_, pa_deadline, timer_cb_, this);
        if (!timer_) {
            roc_panic("pulseaudio sink: can't create timer");
        }
    } else {
        pa_context_rttime_restart(context_, timer_, pa_deadline);
    }
}

bool PulseaudioSink::stop_timer_() {
    if (!timer_) {
        return false;
    }

    pa_context_rttime_restart(context_, timer_, PA_USEC_INVALID);

    const bool expired = core::timestamp() >= timer_deadline_;

    return expired;
}

void PulseaudioSink::timer_cb_(pa_mainloop_api*,
                               pa_time_event*,
                               const struct timeval*,
                               void* userdata) {
    roc_log(LogTrace, "pulseaudio sink: timer callback");

    PulseaudioSink& self = *(PulseaudioSink*)userdata;

    pa_threaded_mainloop_signal(self.mainloop_, 0);
}

} // namespace sndio
} // namespace roc
