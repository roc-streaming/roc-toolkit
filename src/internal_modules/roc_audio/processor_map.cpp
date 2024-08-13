/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/processor_map.h"
#include "roc_audio/beep_plc.h"
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
IResampler* resampler_ctor_fn(const ResamplerConfig& config,
                              const SampleSpec& in_spec,
                              const SampleSpec& out_spec,
                              FrameFactory& frame_factory,
                              core::IArena& arena,
                              void* backend_owner) {
    return new (arena) T(config, in_spec, out_spec, frame_factory, arena);
}

template <class T>
IResampler* decim_resampler_ctor_fn(const ResamplerConfig& config,
                                    const SampleSpec& in_spec,
                                    const SampleSpec& out_spec,
                                    FrameFactory& frame_factory,
                                    core::IArena& arena,
                                    void* backend_owner) {
    core::SharedPtr<IResampler> inner_resampler =
        new (arena) T(config, in_spec, out_spec, frame_factory, arena);

    return new (arena)
        DecimationResampler(inner_resampler, in_spec, out_spec, frame_factory, arena);
}

template <class T>
IPlc* plc_ctor_fn(const PlcConfig& config,
                  const SampleSpec& sample_spec,
                  FrameFactory& frame_factory,
                  core::IArena& arena,
                  void* backend_owner) {
    return new (arena) T(config, sample_spec, frame_factory, arena);
}

} // namespace

ProcessorMap::ProcessorMap(core::IArena& arena)
    : node_pool_("processor_node_pool", arena)
    , node_map_(arena) {
    // resampler
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

    // plc
    {
        Node* node = allocate_builtin_node_();
        node->type = NodeType_Plc;
        node->id = PlcBackend_Beep;
        node->ctor_fn = (void*)(PlcFunc)&plc_ctor_fn<BeepPlc>;
        register_builtin_node_(node);
    }
}

bool ProcessorMap::has_resampler_backend(ResamplerBackend backend_id) const {
    core::Mutex::Lock lock(mutex_);

    return find_node_(NodeType_Resampler, backend_id) != NULL;
}

IResampler* ProcessorMap::new_resampler(const ResamplerConfig& config,
                                        const SampleSpec& in_spec,
                                        const SampleSpec& out_spec,
                                        FrameFactory& frame_factory,
                                        core::IArena& arena) {
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
    return ((ResamplerFunc)node->ctor_fn)(config, in_spec, out_spec, frame_factory, arena,
                                          node->owner);
}

bool ProcessorMap::has_plc_backend(PlcBackend backend_id) const {
    core::Mutex::Lock lock(mutex_);

    return find_node_(NodeType_Plc, backend_id) != NULL;
}

status::StatusCode
ProcessorMap::register_plc(int backend_id, void* backend_owner, PlcFunc ctor_fn) {
    core::Mutex::Lock lock(mutex_);

    roc_log(LogDebug, "processor map: registering plc backend: backend_id=%d",
            backend_id);

    if (backend_id < MinBackendId || backend_id > MaxBackendId) {
        roc_log(LogError,
                "processor map: failed to register plc backend:"
                " invalid backend id: must be in range [%d; %d]",
                MinBackendId, MaxBackendId);
        return status::StatusBadArg;
    }

    if (find_node_(NodeType_Plc, backend_id)) {
        roc_log(LogError,
                "processor map: failed to register plc backend:"
                " backend id %d already exists",
                backend_id);
        return status::StatusBadArg;
    }

    if (!backend_owner) {
        roc_log(LogError,
                "processor map: failed to register plc backend:"
                " backend owner is null");
        return status::StatusBadArg;
    }

    if (!ctor_fn) {
        roc_log(LogError,
                "processor map: failed to register plc backend:"
                " ctor function is null");
        return status::StatusBadArg;
    }

    core::SharedPtr<Node> node = new (node_pool_) Node(node_pool_);
    if (!node) {
        roc_log(LogError,
                "processor map: failed to register plc backend:"
                " pool allocation failed");
        return status::StatusNoMem;
    }

    node->type = NodeType_Plc;
    node->id = backend_id;
    node->owner = backend_owner;
    node->ctor_fn = (void*)ctor_fn;

    if (!node_map_.insert(*node)) {
        roc_log(LogError,
                "processor map: failed to register plc backend:"
                " hashmap allocation failed");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

IPlc* ProcessorMap::new_plc(const PlcConfig& config,
                            const SampleSpec& sample_spec,
                            FrameFactory& frame_factory,
                            core::IArena& arena) {
    core::SharedPtr<Node> node;

    {
        core::Mutex::Lock lock(mutex_);

        if (!(node = find_node_(NodeType_Plc, config.backend))) {
            roc_log(LogError, "processor map: unsupported plc backend: [%d] %s",
                    config.backend, plc_backend_to_str((PlcBackend)config.backend));
            return NULL;
        }
    }

    roc_panic_if(!node->ctor_fn);
    return ((PlcFunc)node->ctor_fn)(config, sample_spec, frame_factory, arena,
                                    node->owner);
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
