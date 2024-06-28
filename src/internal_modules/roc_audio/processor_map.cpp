/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/processor_map.h"
#include "roc_audio/builtin_resampler.h"
#include "roc_audio/decimation_resampler.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#ifdef ROC_TARGET_SPEEXDSP
#include "roc_audio/speex_resampler.h"
#endif // ROC_TARGET_SPEEXDSP

namespace roc {
namespace audio {

namespace {

template <class T>
IResampler* resampler_ctor_fn(void* backend_owner,
                              core::IArena& arena,
                              FrameFactory& frame_factory,
                              const ResamplerConfig& config,
                              const SampleSpec& in_spec,
                              const SampleSpec& out_spec) {
    return new (arena) T(arena, frame_factory, config, in_spec, out_spec);
}

template <class T>
IResampler* decim_resampler_ctor_fn(void* backend_owner,
                                    core::IArena& arena,
                                    FrameFactory& frame_factory,
                                    const ResamplerConfig& config,
                                    const SampleSpec& in_spec,
                                    const SampleSpec& out_spec) {
    core::SharedPtr<IResampler> inner_resampler =
        new (arena) T(arena, frame_factory, config, in_spec, out_spec);

    return new (arena)
        DecimationResampler(inner_resampler, arena, frame_factory, in_spec, out_spec);
}

} // namespace

ProcessorMap::ProcessorMap(core::IArena& arena)
    : node_pool_("processor_node_pool", arena)
    , node_map_(arena) {
#ifdef ROC_TARGET_SPEEXDSP
    {
        Node* node = allocate_builtin_node_();
        node->type = NodeType_Resampler;
        node->id = ResamplerBackend_Speex;
        node->ctor_fn = (void*)(ResamplerFunc)&resampler_ctor_fn<SpeexResampler>;
        register_builtin_node_(node);
    }
    {
        Node* node = allocate_builtin_node_();
        node->type = NodeType_Resampler;
        node->id = ResamplerBackend_SpeexDec;
        node->ctor_fn = (void*)(ResamplerFunc)&decim_resampler_ctor_fn<SpeexResampler>;
        register_builtin_node_(node);
    }
#endif // ROC_TARGET_SPEEXDSP
    {
        Node* node = allocate_builtin_node_();
        node->type = NodeType_Resampler;
        node->id = ResamplerBackend_Builtin;
        node->ctor_fn = (void*)(ResamplerFunc)&resampler_ctor_fn<BuiltinResampler>;
        register_builtin_node_(node);
    }
}

bool ProcessorMap::has_resampler_backend(ResamplerBackend backend_id) const {
    core::Mutex::Lock lock(mutex_);

    return find_node_(NodeType_Resampler, backend_id) != NULL;
}

IResampler* ProcessorMap::new_resampler(core::IArena& arena,
                                        FrameFactory& frame_factory,
                                        const ResamplerConfig& config,
                                        const SampleSpec& in_spec,
                                        const SampleSpec& out_spec) {
    core::SharedPtr<Node> node;

    {
        core::Mutex::Lock lock(mutex_);

        if (!(node = find_node_(NodeType_Resampler, config.backend))) {
            roc_log(LogError, "processor map: unsupported resampler backend: [%d] %s",
                    config.backend, resampler_backend_to_str(config.backend));
            return NULL;
        }
    }

    roc_panic_if(!node->ctor_fn);
    return ((ResamplerFunc)node->ctor_fn)(node->owner, arena, frame_factory, config,
                                          in_spec, out_spec);
}

core::SharedPtr<ProcessorMap::Node> ProcessorMap::find_node_(NodeType type,
                                                             int id) const {
    return node_map_.find(NodeKey(type, id));
}

ProcessorMap::Node* ProcessorMap::allocate_builtin_node_() {
    Node* node = new (node_pool_) Node(node_pool_);
    if (!node) {
        roc_panic("processor map: failed to allocate builtin node");
    }

    return node;
}

void ProcessorMap::register_builtin_node_(Node* node) {
    roc_panic_if_msg(node->type == NodeType_Invalid || node->id < 0,
                     "processor map: invalid builtin node");

    if (!node_map_.insert(*node)) {
        roc_panic("processor map: failed to register builtin node");
    }
}

} // namespace audio
} // namespace roc
