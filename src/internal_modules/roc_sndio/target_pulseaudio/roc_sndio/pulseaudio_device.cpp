/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pulseaudio_device.h"
#include "roc_audio/channel_defs.h"
#include "roc_audio/format.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

const core::nanoseconds_t ReportInterval = 10 * core::Second;

// 60ms is known to work well with majority of sound cards and pulseaudio
// however, on many sound cards you may use lower latencies, e.g.
// 40ms or 20ms, and sometimes even 10ms
const core::nanoseconds_t DefaultLatency = core::Millisecond * 60;

// 10ms is rather high, but works well even on cheap sound cards and CPUs.
// Usually you can use much lower values.
const core::nanoseconds_t DefaultFrameLength = 10 * core::Millisecond;

const core::nanoseconds_t MinTimeout = core::Millisecond * 50;
const core::nanoseconds_t MaxTimeout = core::Second * 2;

audio::PcmSubformat from_pulse_format(pa_sample_format fmt) {
    switch (fmt) {
    case PA_SAMPLE_U8:
        return audio::PcmSubformat_UInt8;

    case PA_SAMPLE_S16LE:
        return audio::PcmSubformat_SInt16_Le;
    case PA_SAMPLE_S16BE:
        return audio::PcmSubformat_SInt16_Be;

    case PA_SAMPLE_S24LE:
        return audio::PcmSubformat_SInt24_Le;
    case PA_SAMPLE_S24BE:
        return audio::PcmSubformat_SInt24_Be;

    case PA_SAMPLE_S24_32LE:
        return audio::PcmSubformat_SInt24_4_Le;
    case PA_SAMPLE_S24_32BE:
        return audio::PcmSubformat_SInt24_4_Be;

    case PA_SAMPLE_S32LE:
        return audio::PcmSubformat_SInt32_Le;
    case PA_SAMPLE_S32BE:
        return audio::PcmSubformat_SInt32_Be;

    case PA_SAMPLE_FLOAT32LE:
        return audio::PcmSubformat_Float32_Le;
    case PA_SAMPLE_FLOAT32BE:
        return audio::PcmSubformat_Float32_Be;

    default:
        break;
    }

    return audio::PcmSubformat_Invalid;
}

pa_sample_format to_pulse_format(audio::PcmSubformat fmt) {
    switch (fmt) {
    case audio::PcmSubformat_UInt8:
    case audio::PcmSubformat_UInt8_Le:
    case audio::PcmSubformat_UInt8_Be:
        return PA_SAMPLE_U8;

    case audio::PcmSubformat_SInt16:
        return PA_SAMPLE_S16NE;
    case audio::PcmSubformat_SInt16_Le:
        return PA_SAMPLE_S16LE;
    case audio::PcmSubformat_SInt16_Be:
        return PA_SAMPLE_S16BE;

    case audio::PcmSubformat_SInt24:
        return PA_SAMPLE_S24NE;
    case audio::PcmSubformat_SInt24_Le:
        return PA_SAMPLE_S24LE;
    case audio::PcmSubformat_SInt24_Be:
        return PA_SAMPLE_S24BE;

    case audio::PcmSubformat_SInt24_4:
        return PA_SAMPLE_S24_32NE;
    case audio::PcmSubformat_SInt24_4_Le:
        return PA_SAMPLE_S24_32LE;
    case audio::PcmSubformat_SInt24_4_Be:
        return PA_SAMPLE_S24_32BE;

    case audio::PcmSubformat_SInt32:
        return PA_SAMPLE_S32NE;
    case audio::PcmSubformat_SInt32_Le:
        return PA_SAMPLE_S32LE;
    case audio::PcmSubformat_SInt32_Be:
        return PA_SAMPLE_S32BE;

    case audio::PcmSubformat_Float32:
        return PA_SAMPLE_FLOAT32NE;
    case audio::PcmSubformat_Float32_Le:
        return PA_SAMPLE_FLOAT32LE;
    case audio::PcmSubformat_Float32_Be:
        return PA_SAMPLE_FLOAT32BE;

    default:
        break;
    }

    return PA_SAMPLE_INVALID;
}

} // namespace

PulseaudioDevice::PulseaudioDevice(audio::FrameFactory& frame_factory,
                                   core::IArena& arena,
                                   const IoConfig& io_config,
                                   DeviceType device_type,
                                   const char* device)
    : IDevice(arena)
    , ISink(arena)
    , ISource(arena)
    , device_type_(device_type)
    , device_(NULL)
    , frame_factory_(frame_factory)
    , sample_spec_(io_config.sample_spec)
    , frame_len_ns_(io_config.frame_length)
    , frame_len_samples_(0)
    , target_latency_ns_(io_config.latency)
    , target_latency_samples_(0)
    , timeout_ns_(0)
    , timeout_samples_(0)
    , record_frag_data_(NULL)
    , record_frag_size_(0)
    , record_frag_flag_(false)
    , open_done_(false)
    , open_status_(status::NoStatus)
    , mainloop_(NULL)
    , context_(NULL)
    , device_info_op_(NULL)
    , stream_(NULL)
    , timer_(NULL)
    , timer_deadline_ns_(0)
    , rate_limiter_(ReportInterval)
    , init_status_(status::NoStatus) {
    if (io_config.sample_spec.has_format()
        && io_config.sample_spec.format() != audio::Format_Pcm) {
        roc_log(LogError,
                "pulseaudio %s: invalid io encoding:"
                " <format> '%s' not supported by backend: spec=%s",
                device_type_to_str(device_type_), io_config.sample_spec.format_name(),
                audio::sample_spec_to_str(io_config.sample_spec).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (io_config.sample_spec.has_subformat()
        && to_pulse_format(io_config.sample_spec.pcm_subformat()) == PA_SAMPLE_INVALID) {
        roc_log(LogError,
                "pulseaudio %s: invalid io encoding:"
                " <subformat> '%s' not supported by backend: spec=%s",
                device_type_to_str(device_type_), io_config.sample_spec.format_name(),
                audio::sample_spec_to_str(io_config.sample_spec).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (frame_len_ns_ == 0) {
        frame_len_ns_ = DefaultFrameLength;
    }

    if (target_latency_ns_ == 0) {
        target_latency_ns_ = DefaultLatency;
    }

    timeout_ns_ = target_latency_ns_ * 2;
    if (timeout_ns_ < MinTimeout) {
        timeout_ns_ = MinTimeout;
    }
    if (timeout_ns_ > MaxTimeout) {
        timeout_ns_ = MaxTimeout;
    }

    roc_log(LogDebug, "pulseaudio %s: opening device: device=%s",
            device_type_to_str(device_type_), device);

    if (device && strcmp(device, "default") != 0) {
        device_ = device;
    }

    if ((init_status_ = start_mainloop_()) != status::StatusOK) {
        return;
    }

    if ((init_status_ = open_()) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

PulseaudioDevice::~PulseaudioDevice() {
    const status::StatusCode code = close();
    if (code != status::StatusOK) {
        roc_log(LogError, "pulseaudio %s: close failed: status=%s",
                device_type_to_str(device_type_), status::code_to_str(code));
    }
}

status::StatusCode PulseaudioDevice::init_status() const {
    return init_status_;
}

DeviceType PulseaudioDevice::type() const {
    return device_type_;
}

ISink* PulseaudioDevice::to_sink() {
    return device_type_ == DeviceType_Sink ? this : NULL;
}

ISource* PulseaudioDevice::to_source() {
    return device_type_ == DeviceType_Source ? this : NULL;
}

audio::SampleSpec PulseaudioDevice::sample_spec() const {
    want_mainloop_();

    pa_threaded_mainloop_lock(mainloop_);

    const audio::SampleSpec sample_spec = sample_spec_;

    pa_threaded_mainloop_unlock(mainloop_);

    return sample_spec;
}

core::nanoseconds_t PulseaudioDevice::frame_length() const {
    want_mainloop_();

    pa_threaded_mainloop_lock(mainloop_);

    const core::nanoseconds_t frame_len = frame_len_ns_;

    pa_threaded_mainloop_unlock(mainloop_);

    return frame_len;
}

bool PulseaudioDevice::has_state() const {
    return true;
}

DeviceState PulseaudioDevice::state() const {
    want_mainloop_();

    pa_threaded_mainloop_lock(mainloop_);

    const DeviceState state =
        open_status_ == status::StatusOK ? DeviceState_Active : DeviceState_Paused;

    pa_threaded_mainloop_unlock(mainloop_);

    return state;
}

status::StatusCode PulseaudioDevice::pause() {
    want_mainloop_();

    close_();

    return status::StatusOK;
}

status::StatusCode PulseaudioDevice::resume() {
    want_mainloop_();

    const status::StatusCode code = open_();

    if (code != status::StatusOK) {
        roc_log(LogError, "pulseaudio %s: can't resume stream: status=%s",
                device_type_to_str(device_type_), status::code_to_str(code));
        return code;
    }

    return status::StatusOK;
}

bool PulseaudioDevice::has_latency() const {
    return true;
}

core::nanoseconds_t PulseaudioDevice::latency() const {
    want_mainloop_();

    pa_threaded_mainloop_lock(mainloop_);

    core::nanoseconds_t latency = 0;

    if (!get_latency_(latency)) {
        // until latency information is retrieved from server first time,
        // assume that actual latency is equal to target latency
        latency = target_latency_ns_;
    }

    pa_threaded_mainloop_unlock(mainloop_);

    return latency;
}

bool PulseaudioDevice::has_clock() const {
    return true;
}

status::StatusCode PulseaudioDevice::rewind() {
    close_();

    const status::StatusCode code = open_();

    if (code != status::StatusOK) {
        roc_log(LogError, "pulseaudio %s: can't restart stream: status=%s",
                device_type_to_str(device_type_), status::code_to_str(code));
        return code;
    }

    return status::StatusOK;
}

void PulseaudioDevice::reclock(core::nanoseconds_t timestamp) {
    // no-op
}

status::StatusCode PulseaudioDevice::read(audio::Frame& frame,
                                          packet::stream_timestamp_t duration,
                                          audio::FrameReadMode mode) {
    roc_panic_if(device_type_ != DeviceType_Source);

    if (!frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(duration))) {
        return status::StatusNoMem;
    }

    frame.set_raw(sample_spec_.is_raw());
    frame.set_duration(duration);

    return handle_request_(frame.bytes(), frame.num_bytes());
}

status::StatusCode PulseaudioDevice::write(audio::Frame& frame) {
    roc_panic_if(device_type_ != DeviceType_Sink);

    return handle_request_(frame.bytes(), frame.num_bytes());
}

status::StatusCode PulseaudioDevice::flush() {
    return status::StatusOK;
}

status::StatusCode PulseaudioDevice::close() {
    roc_log(LogDebug, "pulseaudio %s: closing device", device_type_to_str(device_type_));

    close_();
    stop_mainloop_();

    return status::StatusOK;
}

void PulseaudioDevice::dispose() {
    arena().dispose_object(*this);
}

status::StatusCode PulseaudioDevice::handle_request_(uint8_t* data, size_t size) {
    want_mainloop_();

    while (size > 0) {
        pa_threaded_mainloop_lock(mainloop_);

        if (open_status_ != status::StatusOK) {
            return open_status_;
        }

        const ssize_t ret = request_stream_(data, size);

        if (ret > 0) {
            data += (size_t)ret;
            size -= (size_t)ret;
        }

        if (size == 0) {
            report_latency_();
        }

        pa_threaded_mainloop_unlock(mainloop_);

        if (ret < 0) {
            roc_log(LogInfo, "pulseaudio %s: restarting stream",
                    device_type_to_str(device_type_));

            close_();

            const status::StatusCode code = open_();

            if (code != status::StatusOK) {
                roc_log(LogError, "pulseaudio %s: can't restart stream: status=%s",
                        device_type_to_str(device_type_), code_to_str(code));

                return code;
            }
        }
    }

    return status::StatusOK;
}

void PulseaudioDevice::want_mainloop_() const {
    if (!mainloop_) {
        roc_panic("pulseaudio %s: can't use unopened device",
                  device_type_to_str(device_type_));
    }
}

status::StatusCode PulseaudioDevice::start_mainloop_() {
    mainloop_ = pa_threaded_mainloop_new();
    if (!mainloop_) {
        roc_log(LogError, "pulseaudio %s: pa_threaded_mainloop_new() failed",
                device_type_to_str(device_type_));
        return status::StatusErrDevice;
    }

    if (int err = pa_threaded_mainloop_start(mainloop_)) {
        roc_log(LogError, "pulseaudio %s: pa_threaded_mainloop_start(): %s",
                device_type_to_str(device_type_), pa_strerror(err));
        return status::StatusErrDevice;
    }

    return status::StatusOK;
}

void PulseaudioDevice::stop_mainloop_() {
    if (!mainloop_) {
        return;
    }

    pa_threaded_mainloop_stop(mainloop_);
    pa_threaded_mainloop_free(mainloop_);

    mainloop_ = NULL;
}

status::StatusCode PulseaudioDevice::open_() {
    pa_threaded_mainloop_lock(mainloop_);

    if (!open_done_) {
        if (!open_context_()) {
            set_open_status_(status::StatusErrDevice);
        }
        while (!open_done_) {
            pa_threaded_mainloop_wait(mainloop_);
        }
    }

    const status::StatusCode code = open_status_;

    pa_threaded_mainloop_unlock(mainloop_);

    return code;
}

void PulseaudioDevice::close_() {
    if (!mainloop_) {
        return;
    }

    pa_threaded_mainloop_lock(mainloop_);

    if (open_done_) {
        stop_timer_();
        close_stream_();
        cancel_device_info_op_();
        close_context_();
    }

    open_done_ = false;
    open_status_ = status::NoStatus;

    pa_threaded_mainloop_unlock(mainloop_);
}

void PulseaudioDevice::set_open_status_(status::StatusCode code) {
    if (code == status::StatusOK) {
        roc_log(LogTrace, "pulseaudio %s: successfully opened device",
                device_type_to_str(device_type_));
    } else {
        roc_log(LogDebug, "pulseaudio %s: failed to open device: status=%s",
                device_type_to_str(device_type_), status::code_to_str(code));
    }

    open_done_ = true;
    open_status_ = code;

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

    if (self.open_done_) {
        return;
    }

    const pa_context_state_t state = pa_context_get_state(context);

    switch (state) {
    case PA_CONTEXT_READY:
        roc_log(LogTrace, "pulseaudio %s: successfully opened context",
                device_type_to_str(self.device_type_));

        if (!self.start_device_info_op_()) {
            self.set_open_status_(status::StatusErrDevice);
        }

        break;

    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        roc_log(LogDebug, "pulseaudio %s: failed to open context",
                device_type_to_str(self.device_type_));

        self.set_open_status_(status::StatusErrDevice);
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
    case DeviceType_Sink:
        device_info_op_ = pa_context_get_sink_info_by_name(
            context_, device_, (pa_sink_info_cb_t)device_info_cb_, this);
        break;

    case DeviceType_Source:
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
        self.set_open_status_(status::StatusErrDevice);
        return;
    }

    roc_log(LogTrace, "pulseaudio %s: successfully retrieved device info",
            device_type_to_str(self.device_type_));

    switch (self.device_type_) {
    case DeviceType_Sink:
        if (!self.load_device_params_((*(const pa_sink_info*)info).sample_spec)) {
            self.set_open_status_(status::StatusErrDevice);
            return;
        }
        if (!self.init_stream_params_((*(const pa_sink_info*)info).sample_spec)) {
            self.set_open_status_(status::StatusErrDevice);
            return;
        }
        break;

    case DeviceType_Source:
        if (!self.load_device_params_((*(const pa_source_info*)info).sample_spec)) {
            self.set_open_status_(status::StatusErrDevice);
            return;
        }
        if (!self.init_stream_params_((*(const pa_source_info*)info).sample_spec)) {
            self.set_open_status_(status::StatusErrDevice);
            return;
        }
        break;
    }

    if (!self.open_stream_()) {
        self.set_open_status_(status::StatusErrDevice);
        return;
    }
}

bool PulseaudioDevice::load_device_params_(const pa_sample_spec& device_spec) {
    if (sample_spec_.format() == audio::Format_Invalid) {
        audio::PcmSubformat fmt = from_pulse_format(device_spec.format);

        if (fmt == audio::PcmSubformat_Invalid) {
            // If don't support device's native format, ask pulseaudio
            // to do conversion to our native format.
            fmt = audio::PcmSubformat_Raw;
        }

        sample_spec_.set_format(audio::Format_Pcm);
        sample_spec_.set_pcm_subformat(fmt);
    }

    if (sample_spec_.sample_rate() == 0) {
        sample_spec_.set_sample_rate(device_spec.rate);
    }

    if (!sample_spec_.channel_set().is_valid()) {
        sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
        sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
        sample_spec_.channel_set().set_count(device_spec.channels);
    }

    if (!sample_spec_.is_complete()) {
        roc_log(LogError,
                "pulseaudio %s: can't determine device sample spec:"
                " sample_spec=%s",
                device_type_to_str(device_type_),
                audio::sample_spec_to_str(sample_spec_).c_str());
        return false;
    }

    frame_len_samples_ = sample_spec_.ns_2_stream_timestamp_delta(frame_len_ns_);
    target_latency_samples_ =
        sample_spec_.ns_2_stream_timestamp_delta(target_latency_ns_);
    timeout_samples_ = sample_spec_.ns_2_stream_timestamp_delta(timeout_ns_);

    if (frame_len_ns_ <= 0 || frame_len_samples_ <= 0) {
        roc_log(LogError,
                "pulseaudio %s: frame size must be > 0:"
                " frame_len=%.3fms frame_len_samples=%ld",
                device_type_to_str(device_type_),
                (double)frame_len_ns_ / core::Millisecond, (long)frame_len_samples_);
        return false;
    }

    if (target_latency_ns_ <= 0 || target_latency_samples_ <= 0) {
        roc_log(LogError,
                "pulseaudio %s: target latency must be > 0:"
                " target_latency=%.3fms target_latency_samples=%ld",
                device_type_to_str(device_type_),
                (double)target_latency_ns_ / core::Millisecond,
                (long)target_latency_samples_);
        return false;
    }

    if (timeout_ns_ <= 0 || timeout_samples_ <= 0) {
        roc_log(LogError,
                "pulseaudio %s: timeout must be > 0:"
                " timeout=%.3fms timeout_samples=%ld",
                device_type_to_str(device_type_), (double)timeout_ns_ / core::Millisecond,
                (long)timeout_samples_);
        return false;
    }

    return true;
}

bool PulseaudioDevice::init_stream_params_(const pa_sample_spec& device_spec) {
    stream_spec_.format = to_pulse_format(sample_spec_.pcm_subformat());
    stream_spec_.rate = (uint32_t)sample_spec_.sample_rate();
    stream_spec_.channels = (uint8_t)sample_spec_.num_channels();

    roc_panic_if(stream_spec_.format == PA_SAMPLE_INVALID);
    roc_panic_if(stream_spec_.rate == 0);
    roc_panic_if(stream_spec_.channels == 0);

    const size_t frame_len_bytes = sample_spec_.stream_timestamp_2_bytes(
        (packet::stream_timestamp_t)frame_len_samples_);
    const size_t target_latency_bytes = sample_spec_.stream_timestamp_2_bytes(
        (packet::stream_timestamp_t)target_latency_samples_);

    switch (device_type_) {
    case DeviceType_Sink:
        buff_attrs_.maxlength = (uint32_t)-1;
        buff_attrs_.tlength = (uint32_t)target_latency_bytes;
        buff_attrs_.prebuf = (uint32_t)-1;
        buff_attrs_.minreq = (uint32_t)frame_len_bytes;
        buff_attrs_.fragsize = 0;
        break;

    case DeviceType_Source:
        buff_attrs_.maxlength = (uint32_t)-1;
        buff_attrs_.tlength = 0;
        buff_attrs_.prebuf = 0;
        buff_attrs_.minreq = 0;
        buff_attrs_.fragsize = (uint32_t)target_latency_bytes;
        break;
    }

    return true;
}

bool PulseaudioDevice::open_stream_() {
    roc_panic_if_not(context_);

    roc_log(LogInfo,
            "pulseaudio %s: opening stream: device=%s n_channels=%lu sample_rate=%lu",
            device_type_to_str(device_type_), device_,
            (unsigned long)sample_spec_.num_channels(),
            (unsigned long)sample_spec_.sample_rate());

    stream_ = pa_stream_new(context_, "Roc", &stream_spec_, NULL);
    if (!stream_) {
        roc_log(LogError, "pulseaudio %s: pa_stream_new(): %s",
                device_type_to_str(device_type_),
                pa_strerror(pa_context_errno(context_)));
        return false;
    }

    const pa_stream_flags_t flags = pa_stream_flags_t(
        // adjust device latency based on requested stream latency
        PA_STREAM_ADJUST_LATENCY
        // periodically send updated latency from server to client
        | PA_STREAM_AUTO_TIMING_UPDATE
        // interpolate actual latency instead of going to server each time
        | PA_STREAM_INTERPOLATE_TIMING);

    pa_stream_set_state_callback(stream_, stream_state_cb_, this);

    switch (device_type_) {
    case DeviceType_Sink: {
        pa_stream_set_write_callback(stream_, stream_request_cb_, this);

        const int err =
            pa_stream_connect_playback(stream_, device_, &buff_attrs_, flags, NULL, NULL);
        if (err != 0) {
            roc_log(LogError, "pulseaudio %s: pa_stream_connect_playback(): %s",
                    device_type_to_str(device_type_), pa_strerror(err));
            return false;
        }
    } break;

    case DeviceType_Source: {
        pa_stream_set_read_callback(stream_, stream_request_cb_, this);

        const int err = pa_stream_connect_record(stream_, device_, &buff_attrs_, flags);
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

ssize_t PulseaudioDevice::request_stream_(uint8_t* data, size_t size) {
    ssize_t ret = 0;

    switch (device_type_) {
    case DeviceType_Sink:
        ret = write_stream_(data, size);
        break;

    case DeviceType_Source:
        ret = read_stream_(data, size);
        break;
    }

    return ret;
}

ssize_t PulseaudioDevice::write_stream_(const uint8_t* data, size_t size) {
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

    const int err = pa_stream_write(stream_, data, size, NULL, 0, PA_SEEK_RELATIVE);
    if (err != 0) {
        roc_log(LogError, "pulseaudio %s: pa_stream_write(): %s",
                device_type_to_str(device_type_), pa_strerror(err));
        return -1;
    }

    return (ssize_t)size;
}

ssize_t PulseaudioDevice::read_stream_(uint8_t* data, size_t size) {
    if (record_frag_size_ == 0) {
        wait_stream_();

        const void* fragment = NULL;
        size_t fragment_size = 0;

        const int err = pa_stream_peek(stream_, &fragment, &fragment_size);
        if (err != 0) {
            roc_log(LogError, "pulseaudio %s: pa_stream_peek(): %s",
                    device_type_to_str(device_type_), pa_strerror(err));
            return -1;
        }

        record_frag_data_ = (const uint8_t*)fragment;
        record_frag_size_ = fragment_size;
        record_frag_flag_ = fragment_size != 0; // whether we need to call drop
    }

    if (size > record_frag_size_) {
        size = record_frag_size_;
    }

    if (size > 0) {
        if (record_frag_data_ != NULL) {
            // data is non-null, size is non-zero, we got samples from buffer
            memcpy(data, record_frag_data_, size);
        } else {
            // data is null, size is non-zero, we got hole
            memset(data, 0, size);
        }
    }

    if (record_frag_data_ != NULL) {
        record_frag_data_ += size;
    }
    record_frag_size_ -= size;

    if (record_frag_size_ == 0 && record_frag_flag_) {
        record_frag_data_ = NULL;
        record_frag_flag_ = false;

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
        const size_t avail_size = device_type_ == DeviceType_Sink
            ? pa_stream_writable_size(stream_)
            : pa_stream_readable_size(stream_);

        if (avail_size == (size_t)-1) {
            roc_log(LogError, "pulseaudio %s: stream is broken",
                    device_type_to_str(device_type_));
            return -1;
        }

        if (avail_size == 0 && timer_expired) {
            roc_log(LogInfo,
                    "pulseaudio %s: stream timeout expired:"
                    " latency=%ld(%.3fms) timeout=%ld(%.3fms)",
                    device_type_to_str(device_type_), (long)target_latency_samples_,
                    (double)target_latency_ns_ / core::Millisecond,
                    (long)timeout_samples_, (double)timeout_ns_ / core::Millisecond);

            if (timeout_ns_ < MaxTimeout) {
                timeout_ns_ *= 2;
                if (timeout_ns_ > MaxTimeout) {
                    timeout_ns_ = MaxTimeout;
                }
                roc_log(LogDebug,
                        "pulseaudio %s: stream timeout increased:"
                        " latency=%ld(%.3fms) timeout=%ld(%.3fms)",
                        device_type_to_str(device_type_), (long)target_latency_samples_,
                        (double)target_latency_ns_ / core::Millisecond,
                        (long)timeout_samples_, (double)timeout_ns_ / core::Millisecond);
            }

            return -1;
        }

        if (avail_size != 0) {
            return (ssize_t)avail_size;
        }

        start_timer_(timeout_ns_);

        pa_threaded_mainloop_wait(mainloop_);

        timer_expired = stop_timer_();
    }
}

void PulseaudioDevice::stream_state_cb_(pa_stream* stream, void* userdata) {
    PulseaudioDevice& self = *(PulseaudioDevice*)userdata;

    roc_log(LogTrace, "pulseaudio %s: stream state callback",
            device_type_to_str(self.device_type_));

    if (self.open_done_) {
        return;
    }

    const pa_stream_state_t state = pa_stream_get_state(stream);

    switch (state) {
    case PA_STREAM_READY:
        roc_log(LogTrace, "pulseaudio %s: successfully opened stream",
                device_type_to_str(self.device_type_));

        self.set_open_status_(status::StatusOK);
        break;

    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
        roc_log(LogError, "pulseaudio %s: failed to open stream",
                device_type_to_str(self.device_type_));

        self.set_open_status_(status::StatusErrDevice);
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

bool PulseaudioDevice::get_latency_(core::nanoseconds_t& result) const {
    pa_usec_t latency_us = 0;
    int negative = 0;

    if (int err = pa_stream_get_latency(stream_, &latency_us, &negative)) {
        roc_log(LogError, "pulseaudio %s: pa_stream_get_latency(): %s",
                device_type_to_str(device_type_), pa_strerror(err));
        return false;
    }

    ssize_t latency = (ssize_t)sample_spec_.bytes_2_stream_timestamp(
        pa_usec_to_bytes(latency_us, &stream_spec_));

    if (negative) {
        latency = -latency;
    }

    result = sample_spec_.fract_samples_per_chan_2_ns((float)latency);
    return true;
}

void PulseaudioDevice::report_latency_() {
    if (!rate_limiter_.allow()) {
        return;
    }

    core::nanoseconds_t latency = 0;

    if (!get_latency_(latency)) {
        return;
    }

    roc_log(LogDebug, "pulseaudio %s: io_latency=%ld(%.3fms)",
            device_type_to_str(device_type_),
            (long)sample_spec_.ns_2_stream_timestamp_delta(latency),
            (double)latency / core::Millisecond);
}

void PulseaudioDevice::start_timer_(core::nanoseconds_t timeout) {
    roc_panic_if_not(context_);

    const core::nanoseconds_t timeout_usec =
        (timeout + core::Microsecond - 1) / core::Microsecond;

    timer_deadline_ns_ =
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

    const bool expired = core::timestamp(core::ClockMonotonic) >= timer_deadline_ns_;

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
