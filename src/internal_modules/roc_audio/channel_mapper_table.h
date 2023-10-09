/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_mapper_table.h
//! @brief Channel mapping tables.

#ifndef ROC_AUDIO_CHANNEL_MAPPER_TABLE_H_
#define ROC_AUDIO_CHANNEL_MAPPER_TABLE_H_

#include "roc_audio/channel_defs.h"
#include "roc_audio/sample.h"

namespace roc {
namespace audio {

//! Defines multiplication coefficient for a pair of channels.
struct ChannelMapRule {
    ChannelPosition out_ch; //!< Index of output channel.
    ChannelPosition in_ch;  //!< Index of input channel.
    sample_t coeff;         //!< Multiplication coefficient.
};

//! Defines multiplication matrix for two channel masks.
//! Instead of defining the whole matrix, it defines a list of pairs of
//! output and input channel numbers and corresponding coefficients.
//! Such representation allows more compact definition in the source
//! code. The actual matrix is built by channel mapper at runtime.
struct ChannelMap {
    const char* name; //!< Mapping name.

    ChannelMask in_mask;  //!< Channel mask of input stream.
    ChannelMask out_mask; //!< Channel mask of output stream.

    //! Transformation rules.
    //! Rules are used to fill channel mapping matrix.
    ChannelMapRule rules[24];
};

//! Defines ordered list of channels.
struct ChannelList {
    ChannelPosition chans[ChanPos_Max + 1]; //!< Channels.
};

//! Number of defined channel mappings.
const size_t chan_map_count = 40;

//! Defines list of mappings between all supported surround channel mask pairs.
//! Channel mapper will search for appropriate mapping in this list,
//! based on input and output channel masks.
extern const ChannelMap chan_maps[chan_map_count];

//! Defines mapping of channel order identifier to list of channel positions
//! in corresponding order.
extern const ChannelList chan_orders[ChanOrder_Max];

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_TABLE_H_
