/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PUBLIC_API_PLUGIN_PLC_H_
#define ROC_PUBLIC_API_PLUGIN_PLC_H_

#include "roc/plugin.h"

#include "roc_audio/iplc.h"
#include "roc_audio/plc_config.h"

namespace roc {
namespace api {

//! PLC backend using roc_plugin_plc function table.
class PluginPlc : public audio::IPlc {
public:
    //! Valid plugin function table.
    static bool validate(roc_plugin_plc* plugin);

    //! Construction function.
    //! @p plugin is roc_plugin_plc.
    static IPlc* construct(const audio::PlcConfig& config,
                           const audio::SampleSpec& sample_spec,
                           audio::FrameFactory& frame_factory,
                           core::IArena& arena,
                           void* plugin);

    //! Initialize.
    PluginPlc(const audio::SampleSpec& sample_spec,
              core::IArena& arena,
              roc_plugin_plc* plugin);

    virtual ~PluginPlc();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Sample specification expected by PLC.
    virtual audio::SampleSpec sample_spec() const;

    //! How many samples before lost frame are needed for interpolation.
    virtual packet::stream_timestamp_t lookbehind_len();

    //! How many samples after lost frame are needed for interpolation.
    virtual packet::stream_timestamp_t lookahead_len();

    //! When next frame has no losses, PLC reader calls this method.
    virtual void process_history(audio::Frame& hist_frame);

    //! When next frame is lost, PLC reader calls this method.
    virtual void process_loss(audio::Frame& lost_frame,
                              audio::Frame* prev_frame,
                              audio::Frame* next_frame);

private:
    roc_plugin_plc* plugin_;
    void* plugin_instance_;
    const audio::SampleSpec sample_spec_;
};

} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_PLUGIN_PLC_H_
