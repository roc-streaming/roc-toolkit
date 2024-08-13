/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "plugin_plc.h"
#include "adapters.h"

#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace api {

bool PluginPlc::validate(roc_plugin_plc* plugin) {
    if (!plugin) {
        roc_log(LogError, "roc_plugin_plc: callback table is null");
        return false;
    }

    if (!plugin->new_cb) {
        roc_log(LogError, "roc_plugin_plc: new_cb is null");
        return false;
    }

    if (!plugin->delete_cb) {
        roc_log(LogError, "roc_plugin_plc: delete_cb is null");
        return false;
    }

    if (!plugin->lookahead_len_cb) {
        roc_log(LogError, "roc_plugin_plc: lookahead_len_cb is null");
        return false;
    }

    if (!plugin->process_history_cb) {
        roc_log(LogError, "roc_plugin_plc: process_history_cb is null");
        return false;
    }

    if (!plugin->process_loss_cb) {
        roc_log(LogError, "roc_plugin_plc: process_loss_cb is null");
        return false;
    }

    return true;
}

audio::IPlc* PluginPlc::construct(const audio::PlcConfig& config,
                                  const audio::SampleSpec& sample_spec,
                                  audio::FrameFactory& frame_factory,
                                  core::IArena& arena,
                                  void* plugin) {
    return new (arena) PluginPlc(sample_spec, arena, (roc_plugin_plc*)plugin);
}

PluginPlc::PluginPlc(const audio::SampleSpec& sample_spec,
                     core::IArena& arena,
                     roc_plugin_plc* plugin)
    : IPlc(arena)
    , plugin_(plugin)
    , plugin_instance_(NULL)
    , sample_spec_(sample_spec) {
    roc_panic_if(!plugin_);
    roc_panic_if(!validate(plugin_));

    roc_media_encoding encoding;
    if (!api::sample_spec_to_user(encoding, sample_spec_)) {
        roc_log(
            LogError,
            "roc_plugin_plc: failed to create plugin instance: unsupported sample spec");
        return;
    }

    plugin_instance_ = plugin_->new_cb(plugin_, &encoding);
    if (!plugin_instance_) {
        roc_log(
            LogError,
            "roc_plugin_plc: failed to create plugin instance: new_cb() returned null");
        return;
    }
}

PluginPlc::~PluginPlc() {
    if (plugin_instance_) {
        plugin_->delete_cb(plugin_instance_);
    }
}

status::StatusCode PluginPlc::init_status() const {
    if (!plugin_instance_) {
        return status::StatusNoPlugin;
    }
    return status::StatusOK;
}

audio::SampleSpec PluginPlc::sample_spec() const {
    return sample_spec_;
}

packet::stream_timestamp_t PluginPlc::lookbehind_len() {
    roc_panic_if(!plugin_);
    roc_panic_if(!plugin_instance_);

    // PluginPlc doesn't need prev_frame, because this feature is not exposed
    // via C API to keep the interface simple. Users can implement ring buffer
    // by themselves.

    return 0;
}

packet::stream_timestamp_t PluginPlc::lookahead_len() {
    roc_panic_if(!plugin_);
    roc_panic_if(!plugin_instance_);

    return (packet::stream_timestamp_t)plugin_->lookahead_len_cb(plugin_instance_);
}

void PluginPlc::process_history(audio::Frame& imp_hist_frame) {
    roc_panic_if(!plugin_);
    roc_panic_if(!plugin_instance_);

    if (!plugin_->process_history_cb) {
        return;
    }

    roc_frame hist_frame;
    memset(&hist_frame, 0, sizeof(hist_frame));

    sample_spec_.validate_frame(imp_hist_frame);
    hist_frame.samples = imp_hist_frame.bytes();
    hist_frame.samples_size = imp_hist_frame.num_bytes();

    plugin_->process_history_cb(plugin_instance_, &hist_frame);
}

void PluginPlc::process_loss(audio::Frame& imp_lost_frame,
                             audio::Frame* imp_prev_frame,
                             audio::Frame* imp_next_frame) {
    roc_panic_if(!plugin_);
    roc_panic_if(!plugin_instance_);

    roc_frame lost_frame;
    roc_frame next_frame;

    memset(&lost_frame, 0, sizeof(lost_frame));
    memset(&next_frame, 0, sizeof(next_frame));

    sample_spec_.validate_frame(imp_lost_frame);
    lost_frame.samples = imp_lost_frame.bytes();
    lost_frame.samples_size = imp_lost_frame.num_bytes();

    if (imp_next_frame) {
        sample_spec_.validate_frame(*imp_next_frame);
        next_frame.samples = imp_next_frame->bytes();
        next_frame.samples_size = imp_next_frame->num_bytes();
    }

    plugin_->process_loss_cb(plugin_instance_, &lost_frame, &next_frame);
}

} // namespace api
} // namespace roc
