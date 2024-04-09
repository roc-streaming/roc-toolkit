/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_set.h
//! @brief Channel set.

#ifndef ROC_AUDIO_CHANNEL_SET_H_
#define ROC_AUDIO_CHANNEL_SET_H_

#include "roc_audio/channel_defs.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace audio {

//! Channel set.
//! Multi-word bitmask with bits corresponding to enabled channels.
//! Meaning of each channel is defined by ChannelLayout.
//! Order of serialized channels is defined by ChannelOrder.
class ChannelSet {
public:
    //! Construct empty channel set.
    ChannelSet();

    //! Construct with given layout and 32-bit channel mask.
    //! @remarks
    //!  The mask defines only first 32 channels. All channels outside of 0-31
    //!  range will be disabled. If you need more channels, construct empty
    //!  channel set and enable channels or channel ranges using setters.
    ChannelSet(ChannelLayout layout, ChannelOrder order, ChannelMask mask);

    //! Check two channel sets for equality.
    bool operator==(const ChannelSet& other) const;

    //! Check two channel sets for equality.
    bool operator!=(const ChannelSet& other) const;

    //! Check if channel set has valid layout and order, and non-zero channels.
    bool is_valid() const;

    //! Unset all fields.
    void clear();

    //! Get channel layout.
    //! @remarks
    //!  Defines meaning of channel numbers (e.g. that channel 0 is front-left).
    ChannelLayout layout() const;

    //! Set layout of the channel set.
    void set_layout(ChannelLayout layout);

    //! Get channel order.
    //! @remarks
    //!  Defines order of serialized channels
    //!  (e.g. that front-left goes before front-right).
    ChannelOrder order() const;

    //! Set order of the channel set.
    void set_order(ChannelOrder order);

    //! Get maximum possible number of channels.
    static size_t max_channels();

    //! Get number of enabled channels.
    size_t num_channels() const;

    //! Check if specific channel is enabled.
    bool has_channel(size_t n) const;

    //! Get index of first enabled channel.
    //! @remarks
    //!  Panics if there are no enabled channels.
    size_t first_channel() const;

    //! Get index of last enabled channel.
    //! @remarks
    //!  Panics if there are no enabled channels.
    size_t last_channel() const;

    //! Check if channel set is equal to given mask.
    //! @remarks
    //!  The mask defines only first 32 channels. If any channels outside of 0-31
    //!  range are enabled in channel set, the method will fail.
    bool is_equal(ChannelMask mask) const;

    //! Check if channel set is sub-set of given mask, or equal to it.
    //! @remarks
    //!  The mask defines only first 32 channels. If any channels outside of 0-31
    //!  range are enabled in channel set, the method will fail.
    bool is_subset(ChannelMask mask) const;

    //! Check if channel set is super-set of given mask, or equal to it.
    //! @remarks
    //!  The mask defines only first 32 channels. If any channels outside of 0-31
    //!  range are enabled in channel set, the method will succeed.
    bool is_superset(ChannelMask mask) const;

    //! Set channel mask to given bitmask.
    //! @remarks
    //!  The mask defines only first 32 channels.
    //!  All channels outside of the 0-31 range are disabled.
    void set_mask(ChannelMask mask);

    //! Set channel mask to all channels from inclusive range.
    //! @remarks
    //!  All channels within range and enabled.
    //!  All other channels are disabled.
    void set_range(size_t from, size_t to);

    //! Set channel mask based on channel count.
    //! @remarks
    //!  Tries to find a mask that looks most appropriate for given channel count.
    //!  Falls back to just enabling first N channels and disabling others.
    void set_count(size_t count);

    //! Enable/disable given channel.
    void toggle_channel(size_t n, bool enabled);

    //! Enable/disable all channels in inclusive range.
    void toggle_channel_range(size_t from, size_t to, bool enabled);

    //! Set channel set to result of bitwise AND operation with another set.
    //! @remarks
    //!  Similar to "&=".
    void bitwise_and(const ChannelSet& other);

    //! Set channel set to result of bitwise OR operation with another set.
    //! @remarks
    //!  Similar to "|=".
    void bitwise_or(const ChannelSet& other);

    //! Set channel set to result of bitwise XOR operation with another set.
    //! @remarks
    //!  Similar to "^=".
    void bitwise_xor(const ChannelSet& other);

    //! Get number of bytes in bit mask.
    size_t num_bytes() const;

    //! Get byte by index from bit mask.
    uint8_t byte_at(size_t n) const;

private:
    typedef uint64_t word_t;

    enum {
        MaxChannels = 1024,
        WordBytes = sizeof(word_t),
        WordBits = WordBytes * 8,
        NumWords = MaxChannels / WordBits
    };

    void clear_chans_();
    void index_chans_();

    word_t words_[NumWords];

    uint16_t num_chans_;
    uint16_t first_chan_;
    uint16_t last_chan_;

    ChannelLayout layout_;
    ChannelOrder order_;
};

//! Format ChannelSet to string.
void format_channel_set(const ChannelSet& ch_set, core::StringBuilder& bld);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_SET_H_
