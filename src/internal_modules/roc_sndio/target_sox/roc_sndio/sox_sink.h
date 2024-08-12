/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox_sink.h
//! @brief SoX sink.

#ifndef ROC_SNDIO_SOX_SINK_H_
#define ROC_SNDIO_SOX_SINK_H_

#include <sox.h>

#include "roc_audio/frame_factory.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/io_config.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace sndio {

//! SoX sink.
//! @remarks
//!  Writes samples to output file or device.
//!  Supports multiple drivers for different file types and audio systems.
class SoxSink : public ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    SoxSink(audio::FrameFactory& frame_factory,
            core::IArena& arena,
            const IoConfig& io_config,
            DriverType driver_type);
    ~SoxSink();

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Open sink.
    ROC_ATTR_NODISCARD status::StatusCode open(const char* driver, const char* path);

    //! Get device type.
    virtual DeviceType type() const;

    //! Try to cast to ISink.
    virtual ISink* to_sink();

    //! Try to cast to ISource.
    virtual ISource* to_source();

    //! Get sample specification of the sink.
    virtual audio::SampleSpec sample_spec() const;

    //! Check if the sink supports state updates.
    virtual bool has_state() const;

    //! Get sink state.
    virtual DeviceState state() const;

    //! Pause sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode pause();

    //! Resume sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode resume();

    //! Check if the sink supports latency reports.
    virtual bool has_latency() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Explicitly close the sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Write frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(audio::Frame& frame);

    //! Flush buffered data, if any.
    virtual ROC_ATTR_NODISCARD status::StatusCode flush();

private:
    status::StatusCode init_names_(const char* driver, const char* path);
    status::StatusCode init_buffer_();

    status::StatusCode open_();
    status::StatusCode write_(const sox_sample_t* samples, size_t n_samples);
    status::StatusCode close_();

    const DriverType driver_type_;
    core::StringBuffer driver_name_;
    core::StringBuffer output_name_;

    sox_format_t* output_;
    sox_signalinfo_t out_signal_;

    core::Array<sox_sample_t> buffer_;
    size_t buffer_size_;
    core::nanoseconds_t frame_length_;
    audio::SampleSpec sample_spec_;

    bool paused_;

    status::StatusCode init_status_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_SINK_H_
