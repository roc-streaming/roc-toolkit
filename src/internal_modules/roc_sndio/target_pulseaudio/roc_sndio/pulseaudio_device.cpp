/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pulseaudio_device.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

namespace {

const core::nanoseconds_t ReportInterval = 10 * core::Second;

const core::nanoseconds_t DefaultLatency = core::Millisecond * 60;

const core::nanoseconds_t MinTimeout = core::Millisecond * 50;
const core::nanoseconds_t MaxTimeout = core::Second * 2;

} // namespace

PulseaudioDevice::PulseaudioDevice(const Config& config, DeviceType device_type)
    : device_type_(device_type)
    , device_(NULL)
    , config_(config)
    , frame_size_(0)
    , record_frame_data_(NULL)
    , record_frame_size_(0)
    , open_done_(false)
    , opened_(false)
    , mainloop_(NULL)
    , context_(NULL)
    , device_info_op_(NULL)
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

PulseaudioDevice::~PulseaudioDevice() {
    roc_log(LogDebug, "pulseaudio %s: closing device", device_type_to_str(device_type_));

    close_();
    stop_mainloop_();
}

bool PulseaudioDevice::open(const char* device) {
    if (mainloop_) {
        roc_panic("pulseaudio %s: can't call open() twice",
                  device_type_to_str(device_type_));
    }

    roc_log(LogDebug, "pulseaudio %s: opening device: device=%s",
            device_type_to_str(device_type_), device);

    if (device && strcmp(device, "default") != 0) {
        device_ = device;
    }

    if (!start_mainloop_()) {
        return false;
    }

    if (!open_()) {
        return false;
    }

    return true;
}

audio::SampleSpec PulseaudioDevice::sample_spec() const {
    want_started_();

    pa_threaded_mainloop_lock(mainloop_);

    want_opened_();

    const audio::SampleSpec sample_spec = config_.sample_spec;

    pa_threaded_mainloop_unlock(mainloop_);

    return sample_spec;
}

core::nanoseconds_t PulseaudioDevice::latency() const {
    want_started_();

    pa_threaded_mainloop_lock(mainloop_);

    want_opened_();

    const core::nanoseconds_t latency = config_.latency;

    pa_threaded_mainloop_unlock(mainloop_);

    return latency;
}

bool PulseaudioDevice::has_clock() const {
    return true;
}

void PulseaudioDevice::request(audio::Frame& frame) {
    want_started_();

    audio::sample_t* data = frame.samples();
    size_t size = frame.num_samples();

    while (size > 0) {
        pa_threaded_mainloop_lock(mainloop_);

        ssize_t ret = 0;

        switch (device_type_) {
        case Device_Sink:
            ret = write_stream_(data, size);
            break;

        case Device_Source:
            ret = read_stream_(data, size);
            break;
        }

        pa_threaded_mainloop_unlock(mainloop_);

        if (ret < 0) {
            roc_log(LogInfo, "pulseaudio %s: restarting stream",
                    device_type_to_str(device_type_));

            close_();

            if (!open_()) {
                roc_panic("pulseaudio %s: can't restart stream",
                          device_type_to_str(device_type_));
            }

            return;
        }

        data += (size_t)ret;
        size -= (size_t)ret;
    }
}

bool PulseaudioDevice::check_stream_params_() const {
    if (config_.sample_spec.num_channels() == 0) {
        roc_log(LogError, "pulseaudio %s: # of channels is zero",
                device_type_to_str(device_type_));
        return false;
    }

    if (frame_size_ == 0) {
        roc_log(LogError, "pulseaudio %s: frame size is zero",
                device_type_to_str(device_type_));
        return false;
    }

    if (latency_ <= 0) {
        roc_log(LogError, "pulseaudio %s: latency should be positive",
                device_type_to_str(device_type_));
        return false;
    }

    return true;
}

void PulseaudioDevice::want_started_() const {
    if (!mainloop_) {
        roc_panic("pulseaudio %s: can't use unopened device",
                  device_type_to_str(device_type_));
    }
}

void PulseaudioDevice::want_opened_() const {
    if (!opened_) {
        roc_panic("pulseaudio %s: can't use unopened device",
                  device_type_to_str(device_type_));
    }
}

bool PulseaudioDevice::start_mainloop_() {
    mainloop_ = pa_threaded_mainloop_new();
    if (!mainloop_) {
        roc_log(LogError, "pulseaudio %s: pa_threaded_mainloop_new() failed",
                device_type_to_str(device_type_));
        return false;
    }

    if (int err = pa_threaded_mainloop_start(mainloop_)) {
        roc_log(LogError, "pulseaudio %s: pa_threaded_mainloop_start(): %s",
                device_type_to_str(device_type_), pa_strerror(err));
        return false;
    }

    return true;
}

void PulseaudioDevice::stop_mainloop_() {
    if (!mainloop_) {
        return;
    }

    pa_threaded_mainloop_stop(mainloop_);
    pa_threaded_mainloop_free(mainloop_);

    mainloop_ = NULL;
}

bool PulseaudioDevice::open_() {
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

void PulseaudioDevice::close_() {
    if (!mainloop_) {
        return;
    }

    pa_threaded_mainloop_lock(mainloop_);

    stop_timer_();
    close_stream_();
    cancel_device_info_op_();
    close_context_();

    open_done_ = false;
    opened_ = false;

    pa_threaded_mainloop_unlock(mainloop_);
}

void PulseaudioDevice::set_opened_(bool opened) {
    if (opened) {
        roc_log(LogTrace, "pulseaudio %s: successfully opened device",
                device_type_to_str(device_type_));
    } else {
        roc_log(LogDebug, "pulseaudio %s: failed to open device",
                device_type_to_str(device_type_));
    }

    open_done_ = true;
    opened_ = opened;

    pa_threaded_mainloop_signal(mainloop_, 0);
}

bool PulseaudioDevice::open_context_() {
    roc_log(LogTrace, "pulseaudio %s: opening context", device_type_to_str(device_type_));

    context_ = pa_context_new(pa_threaded_mainloop_get_api(mainloop_), "Roc");
    if (!context_) {
        roc_log(LogError, "pulseaudio %s: pa_context_new() failed",
                device_type_to_str(device_type_));
        return false;
    }

    pa_context_set_state_callback(context_, context_state_cb_, this);

    if (int err = pa_context_connect(context_, NULL, PA_CONTEXT_NOFLAGS, NULL)) {
        roc_log(LogDebug, "pulseaudio %s: pa_context_connect(): %s",
                device_type_to_str(device_type_), pa_strerror(err));
        return false;
    }

    return true;
}

void PulseaudioDevice::close_context_() {
    if (!context_) {
        return;
    }

    roc_log(LogTrace, "pulseaudio %s: closing context", device_type_to_str(device_type_));

    pa_context_disconnect(context_);
    pa_context_unref(context_);

    context_ = NULL;
}

void PulseaudioDevice::context_state_cb_(pa_context* context, void* userdata) {
    PulseaudioDevice& self = *(PulseaudioDevice*)userdata;

    roc_log(LogTrace, "pulseaudio %s: context state callback",
            device_type_to_str(self.device_type_));

    if (self.opened_) {
        return;
    }

    const pa_context_state_t state = pa_context_get_state(context);

    switch (state) {
    case PA_CONTEXT_READY:
        roc_log(LogTrace, "pulseaudio %s: successfully opened context",
                device_type_to_str(self.device_type_));

        if (!self.start_device_info_op_()) {
            self.set_opened_(false);
        }

        break;

    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        roc_log(LogDebug, "pulseaudio %s: failed to open context",
                device_type_to_str(self.device_type_));

        self.set_opened_(false);
        break;

    default:
        roc_log(LogTrace, "pulseaudio %s: ignoring unknown context state",
                device_type_to_str(self.device_type_));
        break;
    }
}

bool PulseaudioDevice::start_device_info_op_() {
    roc_panic_if(device_info_op_);

    roc_log(LogTrace, "pulseaudio %s: requesting device info",
            device_type_to_str(device_type_));

    switch (device_type_) {
    case Device_Sink:
        device_info_op_ = pa_context_get_sink_info_by_name(
            context_, device_, (pa_sink_info_cb_t)device_info_cb_, this);
        break;

    case Device_Source:
        device_info_op_ = pa_context_get_source_info_by_name(
            context_, device_, (pa_source_info_cb_t)device_info_cb_, this);
        break;
    }

    return device_info_op_;
}

void PulseaudioDevice::cancel_device_info_op_() {
    if (!device_info_op_) {
        return;
    }

    pa_operation_cancel(device_info_op_);
    pa_operation_unref(device_info_op_);

    device_info_op_ = NULL;
}

void PulseaudioDevice::device_info_cb_(pa_context*,
                                       const void* info,
                                       int,
                                       void* userdata) {
    PulseaudioDevice& self = *(PulseaudioDevice*)userdata;

    self.cancel_device_info_op_();

    if (!info) {
        roc_log(LogDebug, "pulseaudio %s: failed to retrieve device info",
                device_type_to_str(self.device_type_));
        self.set_opened_(false);
        return;
    }

    roc_log(LogTrace, "pulseaudio %s: successfully retrieved device info",
            device_type_to_str(self.device_type_));

    switch (self.device_type_) {
    case Device_Sink:
        self.init_stream_params_((*(const pa_sink_info*)info).sample_spec);
        break;

    case Device_Source:
        self.init_stream_params_((*(const pa_source_info*)info).sample_spec);
        break;
    }

    if (!self.check_stream_params_()) {
        self.set_opened_(false);
        return;
    }

    if (!self.open_stream_()) {
        self.set_opened_(false);
        return;
    }
}

void PulseaudioDevice::init_stream_params_(const pa_sample_spec& device_sample_spec) {
    if (config_.sample_spec.sample_rate() == 0) {
        config_.sample_spec.set_sample_rate((size_t)device_sample_spec.rate);
    }

    if (frame_size_ == 0) {
        frame_size_ = config_.sample_spec.ns_2_samples_overall(config_.frame_length);
    }

    roc_panic_if(sizeof(audio::sample_t) != sizeof(float));

    sample_spec_.format = PA_SAMPLE_FLOAT32LE;
    sample_spec_.rate = (uint32_t)config_.sample_spec.sample_rate();
    sample_spec_.channels = (uint8_t)config_.sample_spec.num_channels();

    const size_t frame_size_bytes = frame_size_ * sizeof(audio::sample_t);

    const size_t latency_bytes =
        config_.sample_spec.ns_2_samples_overall(latency_) * sizeof(audio::sample_t);

    buffer_attrs_.maxlength = (uint32_t)-1;
    buffer_attrs_.tlength = (uint32_t)latency_bytes;
    buffer_attrs_.prebuf = (uint32_t)-1;
    buffer_attrs_.minreq = (uint32_t)frame_size_bytes;
    buffer_attrs_.fragsize = (uint32_t)-1;
}

bool PulseaudioDevice::open_stream_() {
    roc_panic_if_not(context_);

    roc_log(LogInfo,
            "pulseaudio %s: opening stream: device=%s n_channels=%lu sample_rate=%lu",
            device_type_to_str(device_type_), device_,
            (unsigned long)config_.sample_spec.num_channels(),
            (unsigned long)config_.sample_spec.sample_rate());

    stream_ = pa_stream_new(context_, "Roc", &sample_spec_, NULL);
    if (!stream_) {
        roc_log(LogError, "pulseaudio %s: pa_stream_new(): %s",
                device_type_to_str(device_type_),
                pa_strerror(pa_context_errno(context_)));
        return false;
    }

    const pa_stream_flags_t flags =
        pa_stream_flags_t(PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE);

    pa_stream_set_state_callback(stream_, stream_state_cb_, this);
    pa_stream_set_latency_update_callback(stream_, stream_latency_cb_, this);

    switch (device_type_) {
    case Device_Sink: {
        pa_stream_set_write_callback(stream_, stream_request_cb_, this);

        const int err = pa_stream_connect_playback(stream_, device_, &buffer_attrs_,
                                                   flags, NULL, NULL);
        if (err != 0) {
            roc_log(LogError, "pulseaudio %s: pa_stream_connect_playback(): %s",
                    device_type_to_str(device_type_), pa_strerror(err));
            return false;
        }
    } break;

    case Device_Source: {
        pa_stream_set_read_callback(stream_, stream_request_cb_, this);

        const int err = pa_stream_connect_record(stream_, device_, &buffer_attrs_, flags);
        if (err != 0) {
            roc_log(LogError, "pulseaudio %s: pa_stream_connect_record(): %s",
                    device_type_to_str(device_type_), pa_strerror(err));
            return false;
        }
    } break;
    }

    return true;
}

void PulseaudioDevice::close_stream_() {
    if (!stream_) {
        return;
    }

    roc_log(LogTrace, "pulseaudio %s: closing stream", device_type_to_str(device_type_));

    pa_stream_disconnect(stream_);
    pa_stream_unref(stream_);

    stream_ = NULL;
}

ssize_t PulseaudioDevice::write_stream_(const audio::sample_t* data, size_t size) {
    want_opened_();

    ssize_t avail_size = wait_stream_();

    if (avail_size == -1) {
        return -1;
    }

    roc_log(LogTrace, "pulseaudio %s: write: requested_size=%lu avail_size=%lu",
            device_type_to_str(device_type_), (unsigned long)size,
            (unsigned long)avail_size);

    if (size > (size_t)avail_size) {
        size = (size_t)avail_size;
    }

    const int err = pa_stream_write(stream_, data, size * sizeof(audio::sample_t), NULL,
                                    0, PA_SEEK_RELATIVE);
    if (err != 0) {
        roc_log(LogError, "pulseaudio %s: pa_stream_write(): %s",
                device_type_to_str(device_type_), pa_strerror(err));
        return -1;
    }

    return (ssize_t)size;
}

ssize_t PulseaudioDevice::read_stream_(audio::sample_t* data, size_t size) {
    want_opened_();

    if (record_frame_size_ == 0) {
        wait_stream_();

        const void* data = NULL;
        size_t data_size = 0;

        const int err = pa_stream_peek(stream_, &data, &data_size);
        if (err != 0) {
            roc_log(LogError, "pulseaudio %s: pa_stream_peek(): %s",
                    device_type_to_str(device_type_), pa_strerror(err));
            return -1;
        }

        record_frame_data_ = (const audio::sample_t*)data;
        record_frame_size_ = data_size / sizeof(audio::sample_t);
    }

    if (size > record_frame_size_) {
        size = record_frame_size_;
    }

    memcpy(data, record_frame_data_, size);

    record_frame_data_ += size;
    record_frame_size_ -= size;

    if (record_frame_size_ == 0) {
        record_frame_data_ = NULL;

        const int err = pa_stream_drop(stream_);
        if (err != 0) {
            roc_log(LogError, "pulseaudio %s: pa_stream_drop(): %s",
                    device_type_to_str(device_type_), pa_strerror(err));
            return -1;
        }
    }

    return (ssize_t)size;
}

ssize_t PulseaudioDevice::wait_stream_() {
    bool timer_expired = false;

    for (;;) {
        const size_t avail_size = device_type_ == Device_Sink
            ? pa_stream_writable_size(stream_)
            : pa_stream_readable_size(stream_);

        if (avail_size == (size_t)-1) {
            roc_log(LogError, "pulseaudio %s: stream is broken",
                    device_type_to_str(device_type_));
            return -1;
        }

        if (avail_size == 0 && timer_expired) {
            roc_log(LogInfo,
                    "pulseaudio %s: stream timeout expired: latency=%ld timeout=%ld",
                    device_type_to_str(device_type_),
                    (long)config_.sample_spec.ns_2_rtp_timestamp(latency_),
                    (long)config_.sample_spec.ns_2_rtp_timestamp(timeout_));

            if (timeout_ < MaxTimeout) {
                timeout_ *= 2;
                if (timeout_ > MaxTimeout) {
                    timeout_ = MaxTimeout;
                }
                roc_log(LogDebug,
                        "pulseaudio %s: stream timeout increased: "
                        "latency=%ld timeout=%ld",
                        device_type_to_str(device_type_),
                        (long)config_.sample_spec.ns_2_rtp_timestamp(latency_),
                        (long)config_.sample_spec.ns_2_rtp_timestamp(timeout_));
            }

            return -1;
        }

        if (avail_size != 0) {
            return (ssize_t)avail_size;
        }

        start_timer_(timeout_);

        pa_threaded_mainloop_wait(mainloop_);

        timer_expired = stop_timer_();
    }
}

void PulseaudioDevice::stream_state_cb_(pa_stream* stream, void* userdata) {
    PulseaudioDevice& self = *(PulseaudioDevice*)userdata;

    roc_log(LogTrace, "pulseaudio %s: stream state callback",
            device_type_to_str(self.device_type_));

    if (self.opened_) {
        return;
    }

    const pa_stream_state_t state = pa_stream_get_state(stream);

    switch (state) {
    case PA_STREAM_READY:
        roc_log(LogTrace, "pulseaudio %s: successfully opened stream",
                device_type_to_str(self.device_type_));

        self.set_opened_(true);
        break;

    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
        roc_log(LogError, "pulseaudio %s: failed to open stream",
                device_type_to_str(self.device_type_));

        self.set_opened_(false);
        break;

    default:
        roc_log(LogTrace, "pulseaudio %s: ignoring unknown stream state",
                device_type_to_str(self.device_type_));
        break;
    }
}

void PulseaudioDevice::stream_request_cb_(pa_stream*, size_t length, void* userdata) {
    PulseaudioDevice& self = *(PulseaudioDevice*)userdata;

    roc_log(LogTrace, "pulseaudio %s: stream request callback",
            device_type_to_str(self.device_type_));

    if (length != 0) {
        pa_threaded_mainloop_signal(self.mainloop_, 0);
    }
}

void PulseaudioDevice::stream_latency_cb_(pa_stream* stream, void* userdata) {
    PulseaudioDevice& self = *(PulseaudioDevice*)userdata;

    roc_log(LogTrace, "pulseaudio %s: stream latency callback",
            device_type_to_str(self.device_type_));

    if (!self.rate_limiter_.allow()) {
        return;
    }

    pa_usec_t latency_us = 0;
    int negative = 0;

    if (int err = pa_stream_get_latency(stream, &latency_us, &negative)) {
        roc_log(LogError, "pulseaudio %s: pa_stream_get_latency(): %s",
                device_type_to_str(self.device_type_), pa_strerror(err));
        return;
    }

    ssize_t latency =
        (ssize_t)(pa_usec_to_bytes(latency_us, &self.sample_spec_)
                  / sizeof(audio::sample_t) / self.config_.sample_spec.num_channels());

    if (negative) {
        latency = -latency;
    }

    roc_log(LogDebug, "pulseaudio %s: stream_latency=%ld",
            device_type_to_str(self.device_type_), (long)latency);
}

void PulseaudioDevice::start_timer_(core::nanoseconds_t timeout) {
    roc_panic_if_not(context_);

    const core::nanoseconds_t timeout_usec =
        (timeout + core::Microsecond - 1) / core::Microsecond;

    timer_deadline_ =
        core::timestamp(core::ClockMonotonic) + timeout_usec * core::Microsecond;

    const pa_usec_t pa_deadline = pa_rtclock_now() + (pa_usec_t)timeout_usec;

    if (!timer_) {
        timer_ = pa_context_rttime_new(context_, pa_deadline, timer_cb_, this);
        if (!timer_) {
            roc_panic("pulseaudio %s: can't create timer",
                      device_type_to_str(device_type_));
        }
    } else {
        pa_context_rttime_restart(context_, timer_, pa_deadline);
    }
}

bool PulseaudioDevice::stop_timer_() {
    if (!timer_) {
        return false;
    }

    pa_context_rttime_restart(context_, timer_, PA_USEC_INVALID);

    const bool expired = core::timestamp(core::ClockMonotonic) >= timer_deadline_;

    return expired;
}

void PulseaudioDevice::timer_cb_(pa_mainloop_api*,
                                 pa_time_event*,
                                 const struct timeval*,
                                 void* userdata) {
    PulseaudioDevice& self = *(PulseaudioDevice*)userdata;

    roc_log(LogTrace, "pulseaudio %s: timer callback",
            device_type_to_str(self.device_type_));

    pa_threaded_mainloop_signal(self.mainloop_, 0);
}

} // namespace sndio
} // namespace roc
