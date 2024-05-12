/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/wav_source.h
//! @brief WAV source.

#ifndef ROC_SNDIO_WAV_SOURCE_H_
#define ROC_SNDIO_WAV_SOURCE_H_

#include <dr_wav.h>

#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_buffer.h"
#include "roc_packet/units.h"
#include "roc_sndio/config.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! WAV source.
class WavSource : public ISource, private core::NonCopyable<> {
public:
    //! Initialize.
    WavSource(core::IArena& arena, const Config& config);

    virtual ~WavSource();

    //! Check if the object was successfully constructed.
    bool is_valid() const;

    //! Open input file or device.
    //!
    //! @b Parameters
    //!  - @p path is input file or device name, "-" for stdin.
    //!
    //! @remarks
    //!  If @p path is NULL, defaults are used.
    bool open(const char* path);

    //! Cast IDevice to ISink.
    virtual ISink* to_sink();

    //! Cast IDevice to ISink.
    virtual ISource* to_source();

    //! Get device type.
    virtual DeviceType type() const;

    //! Get device state.
    virtual DeviceState state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Get latency of the source.
    virtual core::nanoseconds_t latency() const;

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Adjust source clock to match consumer clock.
    virtual void reclock(core::nanoseconds_t timestamp);

    //! Read frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(audio::Frame& frame);

private:
    bool open_(const char* path);
    void close_();

    drwav wav_;
    bool file_opened_;
    bool eof_;

    bool valid_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_WAV_SOURCE_H_
