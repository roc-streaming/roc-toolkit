/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_mapper_matrix.h
//! @brief Surround to surround conversation coefficients.

#ifndef ROC_AUDIO_CHANNEL_MAPPER_MATRIX_H_
#define ROC_AUDIO_CHANNEL_MAPPER_MATRIX_H_

#include "roc_audio/channel_mapper_table.h"
#include "roc_audio/channel_set.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Contain conversation coefficients while mapping surround to surround.
class ChannelMapperMatrix : public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @remarks
    //!  Should be used only when mapping surround to surround.
    ChannelMapperMatrix(const ChannelSet& in_chans, const ChannelSet& out_chans);

    //! Return a conversation coefficient for input and output channels.
    sample_t coeff(size_t out_ch, size_t in_ch) const;

private:
    //! Contain mapping of a channel to its position in matrix.
    struct Mapping : public core::NonCopyable<> {
        //! Map channels from @p chs to its position in matrix, taking into account
        //! the channel order. @see ChannelOrder.
        explicit Mapping(const ChannelSet& chs);

        ChannelSet index_set;
        size_t index_map[ChanPos_Max];
    };

    static const ChannelMap* find_channel_map_(const Mapping& out_mapping,
                                               const Mapping& in_mapping,
                                               bool& is_reverse);

    //! Each channel is mapped only to itself.
    void set_fallback_(const Mapping& out_mapping, const Mapping& in_mapping);

    //! Fill mapping matrix based on rules from @p map.
    void set_map_(const ChannelMap& map,
                  bool is_reverse,
                  const Mapping& out_mapping,
                  const Mapping& in_mapping);

    //! Normalize mapping matrix.
    void normalize_();

    void set_(size_t out_ch,
              size_t in_ch,
              sample_t value,
              const Mapping& out_mapping,
              const Mapping& in_mapping);

    sample_t matrix_[ChanPos_Max][ChanPos_Max];
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_MATRIX_H_
