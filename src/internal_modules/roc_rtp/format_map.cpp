/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/format_map.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

FormatMap::FormatMap(core::IArena& arena)
    : node_pool_("format_pool", arena)
    , node_map_(arena) {
    {
        Format fmt;
        fmt.payload_type = PayloadType_L16_Mono;
        fmt.pcm_format =
            audio::PcmFormat(audio::PcmEncoding_SInt16, audio::PcmEndian_Big);
        fmt.sample_spec =
            audio::SampleSpec(44100, audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                              audio::ChanMask_Surround_Mono);
        fmt.packet_flags = packet::Packet::FlagAudio;
        fmt.new_encoder = &audio::PcmEncoder::construct;
        fmt.new_decoder = &audio::PcmDecoder::construct;

        add_builtin_(fmt);
    }
    {
        Format fmt;
        fmt.payload_type = PayloadType_L16_Stereo;
        fmt.pcm_format =
            audio::PcmFormat(audio::PcmEncoding_SInt16, audio::PcmEndian_Big);
        fmt.sample_spec =
            audio::SampleSpec(44100, audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                              audio::ChanMask_Surround_Stereo);
        fmt.packet_flags = packet::Packet::FlagAudio;
        fmt.new_encoder = &audio::PcmEncoder::construct;
        fmt.new_decoder = &audio::PcmDecoder::construct;

        add_builtin_(fmt);
    }
}

const Format* FormatMap::find_by_pt(unsigned int pt) const {
    core::Mutex::Lock lock(mutex_);

    if (core::SharedPtr<Node> node = node_map_.find(pt)) {
        return &node->format;
    }

    return NULL;
}

const Format* FormatMap::find_by_spec(const audio::SampleSpec& spec) const {
    core::Mutex::Lock lock(mutex_);

    for (core::SharedPtr<Node> node = node_map_.front(); node != NULL;
         node = node_map_.nextof(*node)) {
        if (node->format.sample_spec == spec) {
            return &node->format;
        }
    }

    return NULL;
}

bool FormatMap::add_format(const Format& fmt) {
    core::Mutex::Lock lock(mutex_);

    if (fmt.payload_type == 0) {
        roc_panic("format map: bad format: invalid payload type");
    }

    if (!fmt.sample_spec.is_valid()) {
        roc_panic("format map: bad format: invalid sample spec");
    }

    if (!fmt.new_encoder || !fmt.new_decoder) {
        roc_panic("format map: bad format: invalid codec functions");
    }

    if (!node_map_.grow()) {
        roc_log(LogError,
                "format map: failed to register format: hashmap allocation failed");
        return false;
    }

    if (node_map_.find(fmt.payload_type)) {
        roc_log(LogError,
                "format map: failed to register format: payload type %u already exists",
                fmt.payload_type);
        return false;
    }

    core::SharedPtr<Node> node = new (node_pool_) Node(node_pool_, fmt);

    if (!node) {
        roc_log(LogError,
                "format map: failed to register format: pool allocation failed");
        return false;
    }

    node_map_.insert(*node);

    return true;
}

void FormatMap::add_builtin_(const Format& fmt) {
    if (!add_format(fmt)) {
        roc_panic("format map: can't add builtin format");
    }
}

} // namespace rtp
} // namespace roc
