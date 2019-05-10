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

#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_core/unique_ptr.h"
#include "roc_packet/units.h"
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
    //!
    //! @b Parameters
    //!  - @p allocator is used to allocate buffers;
    //!  - @p frame_size defines number of samples per channel in output buffers.
    SoxSource(core::IAllocator& allocator,
              packet::channel_mask_t channels,
              size_t sample_rate,
              size_t frame_size);

    virtual ~SoxSource();

    //! Open input file or device.
    //!
    //! @b Parameters
    //!  - @p driver is input driver name;
    //!  - @p input is input file or device name, "-" for stdin.
    //!
    //! @remarks
    //!  If @p driver or @p input are NULL, defaults are used.
    //!
    //! @pre
    //!  Should be called once before calling start().
    bool open(const char* driver, const char* input);

    //! Get sample rate of an input file or a device.
    //!
    //! @pre
    //!  Input file or device should be opened.
    size_t sample_rate() const;

    //! Returns true if input is a real file.
    //!
    //! @pre
    //!  Input file or device should be opened.
    bool is_file() const;

    //! Returns recommended frame size.
    size_t frame_size() const;

    //! Get current source state.
    virtual State state() const;

    //! Wait until the source state becomes active.
    virtual void wait_active() const;

    //! Read frame.
    virtual bool read(audio::Frame&);

private:
    bool prepare_();
    bool open_(const char* driver, const char* input);
    void close_();

    sox_format_t* input_;
    sox_signalinfo_t in_signal_;
    size_t n_channels_;

    core::IAllocator& allocator_;

    core::UniquePtr<sox_sample_t> buffer_;
    size_t buffer_size_;

    bool is_file_;
    bool eof_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_SOURCE_H_
