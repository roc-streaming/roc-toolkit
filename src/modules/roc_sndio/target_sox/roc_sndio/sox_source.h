/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox_source.h
//! @brief SoX source.

#ifndef ROC_SNDIO_SOX_SOURCE_H_
#define ROC_SNDIO_SOX_SOURCE_H_

#include <sox.h>

#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_buffer.h"
#include "roc_packet/units.h"
#include "roc_sndio/config.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! SoX source.
//! @remarks
//!  Reads samples from input file or device.
//!  Supports multiple drivers for different file types and audio systems.
class SoxSource : public ISource, private core::NonCopyable<> {
public:
    //! Initialize.
    SoxSource(core::IAllocator& allocator, const Config& config);

    virtual ~SoxSource();

    //! Check if the object was successfully constructed.
    bool valid() const;

    //! Open input file or device.
    //!
    //! @b Parameters
    //!  - @p driver is input driver name;
    //!  - @p input is input file or device name, "-" for stdin.
    //!
    //! @remarks
    //!  If @p driver or @p input are NULL, defaults are used.
    bool open(const char* driver, const char* input);

    //! Get sample rate of an input file or a device.
    virtual size_t sample_rate() const;

    //! Get number of channels for the source.
    virtual size_t num_channels() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Get current source state.
    virtual State state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Read frame.
    virtual ssize_t read(audio::Frame&);

private:
    bool setup_names_(const char* driver, const char* input);
    bool setup_buffer_();

    bool open_();
    void close_();

    bool seek_(uint64_t offset);

    core::StringBuffer<16> driver_name_;
    core::StringBuffer<> input_name_;

    core::Array<sox_sample_t> buffer_;
    size_t buffer_size_;
    core::nanoseconds_t frame_length_;
    packet::channel_mask_t channels_;

    sox_format_t* input_;
    sox_signalinfo_t in_signal_;
    size_t n_channels_;

    bool is_file_;
    bool eof_;
    bool paused_;
    bool valid_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_SOURCE_H_
