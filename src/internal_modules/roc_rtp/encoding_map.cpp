/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/encoding_map.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_audio/sample_format.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace rtp {

EncodingMap::EncodingMap(core::IArena& arena)
    : node_pool_("encoding_pool", arena)
    , node_map_(arena) {
    {
        Encoding enc;
        enc.payload_type = PayloadType_L16_Mono;
        enc.sample_spec = audio::SampleSpec(
            44100, audio::PcmFormat_SInt16_Be, audio::ChanLayout_Surround,
            audio::ChanOrder_Smpte, audio::ChanMask_Surround_Mono);
        enc.packet_flags = packet::Packet::FlagAudio;

        register_builtin_encoding_(enc);
    }
    {
        Encoding enc;
        enc.payload_type = PayloadType_L16_Stereo;
        enc.sample_spec = audio::SampleSpec(
            44100, audio::PcmFormat_SInt16_Be, audio::ChanLayout_Surround,
            audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo);
        enc.packet_flags = packet::Packet::FlagAudio;

        register_builtin_encoding_(enc);
    }
}

const Encoding* EncodingMap::find_by_pt(unsigned int pt) const {
    core::Mutex::Lock lock(mutex_);

    if (core::SharedPtr<Node> node = node_map_.find(pt)) {
        return &node->encoding;
    }

    return NULL;
}

const Encoding* EncodingMap::find_by_spec(const audio::SampleSpec& spec) const {
    core::Mutex::Lock lock(mutex_);

    for (core::SharedPtr<Node> node = node_map_.front(); node != NULL;
         node = node_map_.nextof(*node)) {
        if (node->encoding.sample_spec == spec) {
            return &node->encoding;
        }
    }

    return NULL;
}

status::StatusCode EncodingMap::register_encoding(Encoding enc) {
    core::Mutex::Lock lock(mutex_);

    roc_log(LogDebug,
            "encoding map: registering encoding: payload_type=%u sample_spec=%s",
            enc.payload_type, audio::sample_spec_to_str(enc.sample_spec).c_str());

    if (enc.payload_type < MinPayloadType || enc.payload_type > MaxPayloadType) {
        roc_log(LogError,
                "encoding map: failed to register encoding:"
                " invalid encoding id: must be in range [%d; %d]",
                MinPayloadType, MaxPayloadType);
        return status::StatusBadArg;
    }

    if (!enc.sample_spec.is_valid()) {
        roc_log(LogError,
                "encoding map: failed to register encoding:"
                " invalid encoding parameters");
        return status::StatusBadArg;
    }

    resolve_codecs_(enc);

    roc_panic_if_msg(!enc.new_encoder || !enc.new_decoder,
                     "encoding map: missing codec functions");

    if (node_map_.find(enc.payload_type)) {
        roc_log(LogError,
                "encoding map: failed to register encoding:"
                " encoding id %u already exists",
                enc.payload_type);
        return status::StatusConflict;
    }

    core::SharedPtr<Node> node = new (node_pool_) Node(node_pool_, enc);

    if (!node) {
        roc_log(LogError,
                "encoding map: failed to register encoding:"
                " pool allocation failed");
        return status::StatusNoMem;
    }

    if (!node_map_.insert(*node)) {
        roc_log(LogError,
                "encoding map: failed to register encoding:"
                " hashmap allocation failed");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

void EncodingMap::register_builtin_encoding_(const Encoding& enc) {
    const status::StatusCode code = register_encoding(enc);

    if (code != status::StatusOK) {
        roc_panic("encoding map: can't add builtin encoding: status=%s",
                  status::code_to_str(code));
    }
}

void EncodingMap::resolve_codecs_(Encoding& enc) {
    if (enc.new_encoder && enc.new_decoder) {
        return;
    }

    switch (enc.sample_spec.sample_format()) {
    case audio::SampleFormat_Pcm:
        if (!enc.new_encoder) {
            enc.new_encoder = &audio::PcmEncoder::construct;
        }
        if (!enc.new_decoder) {
            enc.new_decoder = &audio::PcmDecoder::construct;
        }
        break;

    case audio::SampleFormat_Invalid:
        break;
    }
}

} // namespace rtp
} // namespace roc
