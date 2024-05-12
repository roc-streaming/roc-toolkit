/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sndfile/roc_sndio/sndfile_source.h
//! @brief Sndfile source.

#ifndef ROC_SNDIO_SNDFILE_SOURCE_H_
#define ROC_SNDIO_SNDFILE_SOURCE_H_

#include <sndfile.h>

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

//! Sndfile source.
//! @remarks
//!  Reads samples from input file.
//!  Supports multiple drivers for different file types.
class SndfileSource : public ISource, private core::NonCopyable<> {
public:
    //! Initialize.
    SndfileSource(core::IArena& arena, const Config& config);

    virtual ~SndfileSource();

    //! Check if the object was successfully constructed.
    bool is_valid() const;

    //! Open input file or device.
    //!
    //! @b Parameters
    //!  - @p driver is input format name;
    //!  - @p path is input file name, "-" for stdin.
    //!
    //! @remarks
    //!  If @p driver is NULL, default is used.
    bool open(const char* driver, const char* path);

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
    virtual ROC_ATTR_NODISCARD status::StatusCode read(audio::Frame&);

private:
    bool open_();
    void close_();

    bool seek_(size_t offset);

    audio::SampleSpec sample_spec_;

    SNDFILE* file_;
    SF_INFO file_info_;
    core::StringBuffer path_;
    bool valid_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SNDFILE_SOURCE_H_
