/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_pulseaudio/roc_sndio/pulseaudio_device.h
//! @brief PulseAudio device.

#ifndef ROC_SNDIO_PULSEAUDIO_DEVICE_H_
#define ROC_SNDIO_PULSEAUDIO_DEVICE_H_

#include <pulse/pulseaudio.h>

#include "roc_audio/frame.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"
#include "roc_sndio/config.h"
#include "roc_sndio/device_state.h"
#include "roc_sndio/device_type.h"

namespace roc {
namespace sndio {

//! PulseAudio device.
//! Base class for PulseAudio source and sink.
class PulseaudioDevice : public core::NonCopyable<> {
public:
    //! Open output device.
    bool open(const char* device);

protected:
    //! Initialize.
    PulseaudioDevice(const Config& config, DeviceType device_type);
    ~PulseaudioDevice();

    //! Get device state.
    DeviceState state() const;

    //! Pause reading.
    void pause();

    //! Resume paused reading.
    bool resume();

    //! Restart reading from the beginning.
    bool restart();

    //! Get sample specification of the sink.
    audio::SampleSpec sample_spec() const;

    //! Get latency of the sink.
    core::nanoseconds_t latency() const;

    //! Process audio frame.
    bool request(audio::Frame& frame);

private:
    static void context_state_cb_(pa_context* context, void* userdata);

    static void
    device_info_cb_(pa_context* context, const void* info, int eol, void* userdata);

    static void stream_state_cb_(pa_stream* stream, void* userdata);
    static void stream_request_cb_(pa_stream* stream, size_t length, void* userdata);

    static void timer_cb_(pa_mainloop_api* mainloop,
                          pa_time_event* timer,
                          const struct timeval* tv,
                          void* userdata);

    bool request_frame_(audio::Frame& frame);

    void want_mainloop_() const;
    bool start_mainloop_();
    void stop_mainloop_();

    bool open_();
    void close_();
    void set_opened_(bool opened);

    bool open_context_();
    void close_context_();

    bool start_device_info_op_();
    void cancel_device_info_op_();

    void init_stream_params_(const pa_sample_spec& device_sample_spec);
    bool check_stream_params_() const;
    bool open_stream_();
    void close_stream_();
    ssize_t request_stream_(audio::sample_t* data, size_t size);
    ssize_t write_stream_(const audio::sample_t* data, size_t size);
    ssize_t read_stream_(audio::sample_t* data, size_t size);
    ssize_t wait_stream_();

    bool get_latency_(core::nanoseconds_t& latency) const;
    void report_latency_();

    void start_timer_(core::nanoseconds_t timeout);
    bool stop_timer_();

    const DeviceType device_type_;
    const char* device_;

    Config config_;
    size_t frame_size_;

    const audio::sample_t* record_frag_data_;
    size_t record_frag_size_;
    bool record_frag_flag_;

    core::nanoseconds_t target_latency_;
    core::nanoseconds_t timeout_;

    bool open_done_;
    bool opened_;

    pa_threaded_mainloop* mainloop_;
    pa_context* context_;
    pa_operation* device_info_op_;
    pa_stream* stream_;
    pa_time_event* timer_;

    core::nanoseconds_t timer_deadline_;

    pa_sample_spec sample_spec_;
    pa_buffer_attr buffer_attrs_;

    core::RateLimiter rate_limiter_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PULSEAUDIO_DEVICE_H_
