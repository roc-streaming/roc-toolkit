/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_mapper_matrix.h
//! @brief Channel mapping matrix.

#ifndef ROC_AUDIO_CHANNEL_MAPPER_MATRIX_H_
#define ROC_AUDIO_CHANNEL_MAPPER_MATRIX_H_

#include "roc_audio/channel_set.h"
#include "roc_audio/channel_tables.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Channel mapping matrix.
//!
//! Used for mapping between two surround layouts. Not used if one or both
//! layouts are multitrack.
//!
//! In surround mapping, every output channel is calculated as a sum of every
//! input channel multiplied by a coefficient from this matrix.
//!
//! Matrix coefficients are defined for physical channel indicies in frame,
//! e.g. coeff(1, 2) defines coefficient for second channel in output frame
//! and third channel in input frame, no matter what is the logical position
//! of the channels (L, R, ...).
//!
//! This  allows to use this matrix not just for mapping between different
//! channel masks, but also for different channel orders, in one operation.
class ChannelMapperMatrix : public core::NonCopyable<> {
public:
    ChannelMapperMatrix();

    //! Build matrix.
    //! @remarks
    //!   Builds matrix based on three tables:
    //!     - two channel order tables
    //!       (define order of input and output channels)
    //!     - channel mapping table
    //!       (defines mapping coefficients between input and output channels)
    void build(const ChannelSet& in_chans, const ChannelSet& out_chans);

    //! Returns coefficient for a pair of input and output indicies.
    //! @remarks
    //!  @p out_index and @p in_index define physical channel offsets
    //!  in audio frame, not their logical positions.
    sample_t coeff(size_t out_index, size_t in_index) const {
        return matrix_[out_index][in_index];
    }

private:
    struct IndexMap {
        ChannelSet index_set;
        size_t index_map[ChanPos_Max];

        IndexMap() {
            memset(index_map, 0, sizeof(index_map));
        }
    };

    const ChannelMapTable* select_mapping_table_(const IndexMap& out_mapping,
                                                 const IndexMap& in_mapping,
                                                 bool& map_reversed);

    void build_index_mapping_(IndexMap& mapping, const ChannelSet& ch_set);

    void build_table_matrix_(const ChannelMapTable& map_table,
                             bool map_reversed,
                             const IndexMap& out_mapping,
                             const IndexMap& in_mapping);

    void build_diagonal_matrix_(const IndexMap& out_mapping, const IndexMap& in_mapping);

    void normalize_matrix_();

    void set_coeff_(size_t out_ch,
                    size_t in_ch,
                    sample_t value,
                    const IndexMap& out_mapping,
                    const IndexMap& in_mapping);

    sample_t matrix_[ChanPos_Max][ChanPos_Max];
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_MATRIX_H_
