/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/processor_map.h
//! @brief Resampler map.

#ifndef ROC_AUDIO_PROCESSOR_MAP_H_
#define ROC_AUDIO_PROCESSOR_MAP_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iplc.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/plc_config.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/allocation_policy.h"
#include "roc_core/hashmap.h"
#include "roc_core/iarena.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/slab_pool.h"
#include "roc_core/stddefs.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Audio processors map.
//! Holds all registered processor implementations and allows to create
//! them using a numeric identifier.
//! Thread-safe.
class ProcessorMap : public core::NonCopyable<> {
public:
    //! Initialize.
    explicit ProcessorMap(core::IArena& arena);

    //! Resampler factory function.
    typedef IResampler* (*ResamplerFunc)(const ResamplerConfig& config,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec,
                                         FrameFactory& frame_factory,
                                         core::IArena& arena,
                                         void* backend_owner);

    //! Check if given backend is supported.
    bool has_resampler_backend(ResamplerBackend backend_id) const;

    //! Instantiate IResampler for given configuration.
    //! The type depends on backend specified in @p config.
    IResampler* new_resampler(const ResamplerConfig& config,
                              const SampleSpec& in_spec,
                              const SampleSpec& out_spec,
                              FrameFactory& frame_factory,
                              core::IArena& arena);

    //! PLC factory function.
    typedef IPlc* (*PlcFunc)(const PlcConfig& config,
                             const SampleSpec& sample_spec,
                             FrameFactory& frame_factory,
                             core::IArena& arena,
                             void* backend_owner);

    //! Register custom PLC backend.
    ROC_ATTR_NODISCARD status::StatusCode
    register_plc(int backend_id, void* backend_owner, PlcFunc ctor_fn);

    //! Check if given backend is supported.
    bool has_plc_backend(PlcBackend backend_id) const;

    //! Instantiate IPlc for given configuration.
    //! The type depends on backend specified in @p config.
    IPlc* new_plc(const PlcConfig& config,
                  const SampleSpec& sample_spec,
                  FrameFactory& frame_factory,
                  core::IArena& arena);

private:
    friend class core::Singleton<ProcessorMap>;

    enum {
        MinBackendId = 1000,
        MaxBackendId = 9999,
    };

    enum { PreallocatedNodes = 16 };

    enum NodeType {
        NodeType_Invalid,
        NodeType_Resampler,
        NodeType_Plc,
    };

    struct NodeKey {
        NodeType type;
        int id;

        NodeKey(NodeType type, int id)
            : type(type)
            , id(id) {
        }
    };

    struct Node : core::RefCounted<Node, core::PoolAllocation>, core::HashmapNode<> {
        Node(core::IPool& pool)
            : core::RefCounted<Node, core::PoolAllocation>(pool)
            , type(NodeType_Invalid)
            , id(-1)
            , owner(NULL)
            , ctor_fn(NULL) {
        }

        NodeType type;
        int id;
        void* owner;
        void* ctor_fn;

        NodeKey key() const {
            return NodeKey(type, id);
        }

        static core::hashsum_t key_hash(const NodeKey& key) {
            core::hashsum_t hash = 0;
            core::hashsum_add(hash, &key.type, sizeof(key.type));
            core::hashsum_add(hash, &key.id, sizeof(key.id));
            return hash;
        }

        static bool key_equal(const NodeKey& key1, const NodeKey& key2) {
            return key1.type == key2.type && key1.id == key2.id;
        }
    };

    core::SharedPtr<Node> find_node_(NodeType type, int id) const;

    Node* allocate_builtin_node_();
    void register_builtin_node_(Node* node);

    core::Mutex mutex_;

    core::SlabPool<Node, PreallocatedNodes> node_pool_;
    core::Hashmap<Node, PreallocatedNodes> node_map_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROCESSOR_MAP_H_
