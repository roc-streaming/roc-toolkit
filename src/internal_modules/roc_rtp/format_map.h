/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/format_map.h
//! @brief RTP payload format map.

#ifndef ROC_RTP_FORMAT_MAP_H_
#define ROC_RTP_FORMAT_MAP_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/allocation_policy.h"
#include "roc_core/attributes.h"
#include "roc_core/hashmap.h"
#include "roc_core/iarena.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ref_counted.h"
#include "roc_core/slab_pool.h"
#include "roc_rtp/format.h"

namespace roc {
namespace rtp {

//! RTP payload format map.
//! Thread-safe.
//! Returned formats are immutable and can be safely used from
//! any thread.
class FormatMap : public core::NonCopyable<> {
public:
    //! Initialize.
    FormatMap(core::IArena& arena);

    //! Find format by payload type.
    //! @returns
    //!  pointer to the format structure or null if there is no format
    //!  registered for this payload type.
    const Format* find_by_pt(unsigned int pt) const;

    //! Find format by sample specification.
    //! @returns
    //!  pointer to the format structure or null if there is no format
    //!  with matching specification.
    const Format* find_by_spec(const audio::SampleSpec& spec) const;

    //! Add format to the map.
    //! @returns
    //!  true if successfully added or false if another format with the same
    //!  payload type already exists.
    ROC_ATTR_NODISCARD bool add_format(const Format& fmt);

private:
    enum { PreallocatedNodes = 16 };

    struct Node : core::RefCounted<Node, core::PoolAllocation>, core::HashmapNode {
        Node(core::IPool& pool, const Format& format)
            : core::RefCounted<Node, core::PoolAllocation>(pool)
            , format(format) {
        }

        Format format;

        unsigned int key() const {
            return format.payload_type;
        }

        static core::hashsum_t key_hash(unsigned int pt) {
            return core::hashsum_int(pt);
        }

        static bool key_equal(unsigned int pt1, unsigned int pt2) {
            return pt1 == pt2;
        }
    };

    void add_builtin_(const Format& fmt);

    core::Mutex mutex_;

    core::SlabPool<Node, PreallocatedNodes> node_pool_;
    core::Hashmap<Node, PreallocatedNodes> node_map_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_FORMAT_MAP_H_
