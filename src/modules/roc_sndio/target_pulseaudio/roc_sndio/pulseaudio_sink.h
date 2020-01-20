/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_pulseaudio/roc_sndio/pulseaudio_sink.h
//! @brief PulseAudio sink.

#ifndef ROC_SNDIO_PULSEAUDIO_SINK_H_
#define ROC_SNDIO_PULSEAUDIO_SINK_H_

#include <pulse/pulseaudio.h>

#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_sndio/config.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace sndio {

//! PulseAudio sink,
class PulseaudioSink : public ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    PulseaudioSink(const Config& config);

    ~PulseaudioSink();

    //! Open output device.
    bool open(const char* device);

    //! Get sample rate of the sink.
    virtual size_t sample_rate() const;

    //! Get number of channels for the sink.
    virtual size_t num_channels() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    static void context_state_cb_(pa_context* context, void* userdata);

    static void
    sink_info_cb_(pa_context* context, const pa_sink_info* info, int eol, void* userdata);

    static void stream_state_cb_(pa_stream* stream, void* userdata);
    static void stream_write_cb_(pa_stream* stream, size_t length, void* userdata);
    static void stream_latency_cb_(pa_stream* stream, void* userdata);

    static void timer_cb_(pa_mainloop_api* mainloop,
                          pa_time_event* timer,
                          const struct timeval* tv,
                          void* userdata);

    bool write_frame_(audio::Frame& frame);

    bool check_params_() const;

    void ensure_started_() const;
    void ensure_opened_() const;

    bool start_mainloop_();
    void stop_mainloop_();

    bool open_();
    void close_();
    void set_opened_(bool opened);

    bool open_context_();
    void close_context_();

    bool start_sink_info_op_();
    void cancel_sink_info_op_();

    void init_stream_params_(const pa_sink_info& info);
    bool open_stream_();
    void close_stream_();
    ssize_t write_stream_(const audio::sample_t* data, size_t size);
    ssize_t wait_stream_();

    void start_timer_(core::nanoseconds_t timeout);
    bool stop_timer_();

    const char* device_;
    size_t sample_rate_;
    const size_t num_channels_;
    const size_t frame_size_;

    core::nanoseconds_t latency_;
    core::nanoseconds_t timeout_;

    bool open_done_;
    bool opened_;

    pa_threaded_mainloop* mainloop_;
    pa_context* context_;
    pa_operation* sink_info_op_;
    pa_stream* stream_;
    pa_time_event* timer_;

    core::nanoseconds_t timer_deadline_;

    pa_sample_spec sample_spec_;
    pa_buffer_attr buffer_attrs_;

    core::RateLimiter rate_limiter_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PULSEAUDIO_SINK_H_
