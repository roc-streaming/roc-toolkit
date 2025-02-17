/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/encoding_map.h
//! @brief RTP encoding map.

#ifndef ROC_RTP_ENCODING_MAP_H_
#define ROC_RTP_ENCODING_MAP_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/allocation_policy.h"
#include "roc_core/attributes.h"
#include "roc_core/hashmap.h"
#include "roc_core/iarena.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ref_counted.h"
#include "roc_core/slab_pool.h"
#include "roc_rtp/encoding.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtp {

//! RTP encoding map.
//! Holds all registered encodings and their properties and codecs.
//! Thread-safe.
//! Returned encodings are immutable and can be safely used from
//! any thread.
class EncodingMap : public core::NonCopyable<> {
public:
    //! Initialize.
    explicit EncodingMap(core::IArena& arena);

    //! Add encoding to the map.
    ROC_ATTR_NODISCARD status::StatusCode register_encoding(Encoding enc);

    //! Find encoding by payload type.
    //! @returns
    //!  pointer to the encoding structure or null if there is no encoding
    //!  registered for this payload type.
    const Encoding* find_by_pt(unsigned int pt) const;

    //! Find encoding by sample specification.
    //! @returns
    //!  pointer to the encoding structure or null if there is no encoding
    //!  with matching specification.
    const Encoding* find_by_spec(const audio::SampleSpec& spec) const;

private:
    enum { PreallocatedNodes = 16 };

    struct Node : core::RefCounted<Node, core::PoolAllocation>, core::HashmapNode<> {
        Node(core::IPool& pool, const Encoding& encoding)
            : core::RefCounted<Node, core::PoolAllocation>(pool)
            , encoding(encoding) {
        }

        Encoding encoding;

        unsigned int key() const {
            return encoding.payload_type;
        }

        static core::hashsum_t key_hash(unsigned int pt) {
            return core::hashsum_int(pt);
        }

        static bool key_equal(unsigned int pt1, unsigned int pt2) {
            return pt1 == pt2;
        }
    };

    void register_builtin_encoding_(const Encoding& enc);
    void resolve_codecs_(Encoding& enc);

    core::Mutex mutex_;

    core::SlabPool<Node, PreallocatedNodes> node_pool_;
    core::Hashmap<Node, PreallocatedNodes> node_map_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_ENCODING_MAP_H_
