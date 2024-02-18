/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_tables.h
//! @brief Channel tables.

#ifndef ROC_AUDIO_CHANNEL_TABLES_H_
#define ROC_AUDIO_CHANNEL_TABLES_H_

#include "roc_audio/channel_defs.h"
#include "roc_audio/sample.h"

namespace roc {
namespace audio {

//! Defines string name for channel position.
struct ChannelPositionName {
    //! Channel name.
    const char* name;

    //! Channel position (numeric identifier).
    ChannelPosition pos;
};

//! Defines string name for pre-defined channel mask.
struct ChannelMaskName {
    //! Mask name.
    const char* name;

    //! Bitmask of enabled channels.
    ChannelMask mask;
};

//! Defines multiplication coefficient for a pair of channels.
struct ChannelMapRule {
    //! Index of output channel.
    ChannelPosition out_ch;

    //! Index of input channel.
    ChannelPosition in_ch;

    //! Multiplication coefficient.
    //! @remarks
    //!  output channel is a sum of input channels multiplied
    //!  by corresponding coefficients.
    sample_t coeff;
};

//! Defines multiplication matrix for two channel masks.
//!
//! Instead of defining the whole matrix, it defines a list of pairs of
//! output and input channel numbers and corresponding coefficients.
//!
//! Such representation allows more compact definition in the source
//! code. The actual matrix is built by channel mapper at runtime.
struct ChannelMapTable {
    //! Table name.
    const char* name;

    //! Channel mask of input stream.
    ChannelMask in_mask;
    //! Channel mask of output stream.
    ChannelMask out_mask;

    //! Transformation rules.
    //! Rules are used to fill channel mapping matrix.
    ChannelMapRule rules[32];
};

//! Defines ordered list of channels.
struct ChannelOrderTable {
    //! Order name.
    const char* name;

    //! Order identifier.
    ChannelOrder order;

    //! List of channels.
    //! Last channel is equal to ChanPos_Max.
    ChannelPosition chans[ChanPos_Max + 1];
};

//! Defines mapping between channel position and its name.
extern const ChannelPositionName ChanPositionNames[ChanPos_Max];

//! Defines mapping between channel mask and its name.
extern const ChannelMaskName ChanMaskNames[17];

//! Defines mapping of channel order identifier to list of channel positions
//! in corresponding order.
//!
//! When channel order is applied, the list of channels is filtered, and only
//! channels present in channel mask are kept. The resulting filtered list
//! defines how channels are placed in memory.
//!
//! This allows us to define single list that for multiple channel masks.
//! For example, ITU/SMPTE defines order for each channel mask (5.x, 7.x),
//! but we define only one list ChanOrder_Smpte, and after filtering it
//! becomes suitable for each of the masks.
//!
//! The opposite is also true: if some channel is missing from the order's
//! list, it is considered unsupported by the order and is zeroized.
//!
//! Links:
//!  https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.2102-0-201701-I!!PDF-E.pdf
extern const ChannelOrderTable ChanOrderTables[ChanOrder_Max];

//! Defines list of mappings between all supported surround channel mask pairs.
//!
//! Channel mapper will search for appropriate mapping in this list,
//! based on input and output channel masks.
//!
//! These tables define downmixing coefficients for mapping between different
//! surround channel sets. They are used for both downmixing and upmixing.
//!
//! Mappings should be ordered from smaller to larger masks, because channel mapper
//! will use the very first pair that covers both output and input masks.
//!
//! Only downmixing mappings are defined. Upmixing mappings are derived
//! automatically from them.
//!
//! Technically, some of the mappings are actually partially downmixing, and
//! partially upmixing, for example mapping from 6.x to 5.1.x downmixes some
//! channels and upmixes others. However, for convenience, we still call it
//! "downmixing" because we consider 6.x to be a "larger" channel set than 5.x.
//!
//! For groups of similar layouts, when possible, mappings are defined only for
//! the most complete layout, and are automatically reused for the rest. For example,
//! mappings for 5.1.2 may be automatically used for 5.1 and 5.0.
//!
//! These tables were originally based on the following documents
//! (and then extended to cover more combinations):
//!  - ITU-R BS.775-1, ANNEX 4
//!  - A/52, Digital Audio Compression (AC-3) (E-AC-3) Standard, sections 6.1.12 and 7.8
//!
//! Useful links:
//!  https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.775-1-199407-S!!PDF-E.pdf
//!  https://prdatsc.wpenginepowered.com/wp-content/uploads/2021/04/A52-2018.pdf
//!  https://www.audiokinetic.com/en/library/edge/?source=Help&id=downmix_tables
//!  https://trac.ffmpeg.org/wiki/AudioChannelManipulation
//!  https://superuser.com/questions/852400
extern const ChannelMapTable ChanMapTables[71];

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_TABLES_H_
