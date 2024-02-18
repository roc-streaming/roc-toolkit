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

#include "roc_audio/channel_defs.h"
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
//! Matrix coefficients are defined for physical channel indices in frame,
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

    //! Returns coefficient for a pair of input and output indices.
    //! @remarks
    //!  @p out_index and @p in_index define physical channel offsets
    //!  in audio frame, not their logical positions.
    sample_t coeff(size_t out_index, size_t in_index) const {
        return index_matrix_[out_index][in_index];
    }

private:
    // Mapping of physical index in frame <=> logical channel position.
    // We create one index map for input and one for output.
    struct IndexMap {
        ChannelSet enabled_chans;
        size_t chan_2_index[ChanPos_Max];
        ChannelPosition index_2_chan[ChanPos_Max];

        IndexMap() {
            memset(chan_2_index, 0, sizeof(chan_2_index));
            memset(index_2_chan, 0, sizeof(index_2_chan));
        }
    };

    // Mapping matrix for downmixing or upmixing from input to output.
    // Uses logical channel positions.
    struct ChannelMap {
        sample_t chan_matrix[ChanPos_Max][ChanPos_Max];

        ChannelMap() {
            memset(chan_matrix, 0, sizeof(chan_matrix));
        }
    };

    void build_index_mapping_(IndexMap& index_map, const ChannelSet& ch_set);

    bool build_channel_mapping_(ChannelMap& result_map,
                                const ChannelSet& in_chans,
                                const ChannelSet& out_chans);

    bool can_downmix_(const ChannelSet& in_chans, const ChannelSet& out_chans);

    const ChannelMapTable* next_downmix_table_(const ChannelSet& in_chans,
                                               const ChannelSet& out_chans);
    const ChannelMapTable* next_upmix_table_(const ChannelSet& in_chans,
                                             const ChannelSet& out_chans);

    void fill_mapping_from_table_(ChannelMap& result_map,
                                  const ChannelMapTable& map_table,
                                  bool is_downmixing,
                                  const ChannelSet& in_chans,
                                  const ChannelSet& out_chans);

    void fill_fallback_mapping_(ChannelMap& result_map,
                                const ChannelSet& in_chans,
                                const ChannelSet& out_chans);

    void combine_mappings_(ChannelMap& result_map, const ChannelMap& next_map);
    void normalize_mapping_(ChannelMap& chan_map);

    void populate_index_matrix_(const IndexMap& in_index_map,
                                const IndexMap& out_index_map,
                                const ChannelMap& chan_map);

    void print_table_matrix_(const ChannelMap& chan_map);
    void print_index_matrix_(const IndexMap& in_index_map, const IndexMap& out_index_map);

    sample_t index_matrix_[ChanPos_Max][ChanPos_Max];
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_MATRIX_H_
