/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/io_pump.h
//! @brief Pump.

#ifndef ROC_SNDIO_IO_PUMP_H_
#define ROC_SNDIO_IO_PUMP_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/atomic.h"
#include "roc_core/attributes.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_sndio/io_config.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! Audio pump.
//! @remarks
//!  Reads frames from source and writes them to sink.
class IoPump : public core::NonCopyable<> {
public:
    //! Pump mode.
    enum Mode {
        // Run until the source return EOF.
        ModePermanent = 0,

        // Run until the source return EOF or become inactive first time.
        ModeOneshot = 1
    };

    //! Initialize.
    IoPump(core::IPool& frame_pool,
           core::IPool& frame_buffer_pool,
           ISource& source,
           ISource* backup_source,
           ISink& sink,
           const IoConfig& io_config,
           Mode mode);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Run the pump.
    //! @remarks
    //!  Run until the stop() is called or, if oneshot mode is enabled,
    //!  the source becomes inactive.
    ROC_ATTR_NODISCARD status::StatusCode run();

    //! Stop the pump.
    //! @remarks
    //!  May be called from any thread.
    void stop();

private:
    status::StatusCode next_();
    status::StatusCode switch_source_(ISource* new_source);
    status::StatusCode transfer_frame_(ISource& source, ISink& sink);
    status::StatusCode flush_sink_();
    status::StatusCode close_all_devices_();

    audio::FrameFactory frame_factory_;

    ISource& main_source_;
    ISource* backup_source_;
    ISource* current_source_;
    ISink& sink_;

    const audio::SampleSpec sample_spec_;

    audio::FramePtr frame_;
    size_t frame_size_;
    packet::stream_timestamp_t frame_duration_;

    const Mode mode_;
    bool was_active_;
    core::Atomic<int> stop_;

    uint64_t transferred_bytes_;

    status::StatusCode init_status_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IO_PUMP_H_
