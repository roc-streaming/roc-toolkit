/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper.h"
#include "roc_audio/channel_set_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

bool is_surround_left(size_t ch) {
    return ch == ChannelPos_Left;
}

bool is_surround_right(size_t ch) {
    return ch == ChannelPos_Right;
}

bool is_surround_center(size_t) {
    return false;
}

// Layouts: mono => mono
// Algorithm:
//  - copy samples
void map_mono_to_mono(const ChannelSet&,
                      const ChannelSet&,
                      const ChannelSet&,
                      const sample_t* in_samples,
                      sample_t* out_samples,
                      size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        *out_samples++ = *in_samples++;
    }
}

// Layouts: mono => surround
// Algorithm:
//  - duplicate input mono channel to all left, right, and center output channels
//  - zeroise all other output channels
void map_mono_to_surround(const ChannelSet&,
                          const ChannelSet& out_chans,
                          const ChannelSet&,
                          const sample_t* in_samples,
                          sample_t* out_samples,
                          size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        for (size_t ch = out_chans.first_channel(); ch <= out_chans.last_channel();
             ch++) {
            if (out_chans.has_channel(ch)) {
                if (is_surround_left(ch) || is_surround_right(ch)
                    || is_surround_center(ch)) {
                    *out_samples = *in_samples;
                } else {
                    *out_samples = 0;
                }
                out_samples++;
            }
        }
        in_samples++;
    }
}

// Layouts: surround => mono
// Algorithm:
//  - set output mono channel to the average of all left, right, and center input channels
//  - ignore all other input channels
void map_surround_to_mono(const ChannelSet& in_chans,
                          const ChannelSet&,
                          const ChannelSet&,
                          const sample_t* in_samples,
                          sample_t* out_samples,
                          size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        sample_t sum = 0;
        size_t cnt = 0;

        for (size_t ch = in_chans.first_channel(); ch <= in_chans.last_channel(); ch++) {
            if (in_chans.has_channel(ch)) {
                if (is_surround_left(ch) || is_surround_right(ch)
                    || is_surround_center(ch)) {
                    sum += *in_samples;
                    cnt++;
                }
                in_samples++;
            }
        }

        if (cnt != 0) {
            *out_samples = sum / cnt;
        } else {
            *out_samples = 0;
        }
        out_samples++;
    }
}

// TODO: implement algorithm for surround sound
// (current algorithm works only for L and R channels)
void map_surround_to_surround(const ChannelSet& in_chans,
                              const ChannelSet& out_chans,
                              const ChannelSet& inout_chans,
                              const sample_t* in_samples,
                              sample_t* out_samples,
                              size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        for (size_t ch = inout_chans.first_channel(); ch <= inout_chans.last_channel();
             ch++) {
            if (in_chans.has_channel(ch)) {
                if (out_chans.has_channel(ch)) {
                    *out_samples++ = *in_samples;
                }
                in_samples++;
            } else {
                if (out_chans.has_channel(ch)) {
                    *out_samples++ = 0;
                }
            }
        }
    }
}

// Layouts: multitrack <=> mono/surround
// Algorithm:
//  - copy first N channels of input to output, where N is the minimum of
//    the input channel count and the output channel count
//  - zeroise extra output channels, if the output channel count is larger
//    than the input channel count
//  - ignore extra input channels, if the input channel count is larger
//    than the output channel count
//  - note that the actual channel indicies in input and output bitmasks are
//    completely ignored; only channel counts are taked into account
void map_multitrack_non_multitrack(const ChannelSet& in_chans,
                                   const ChannelSet& out_chans,
                                   const ChannelSet&,
                                   const sample_t* in_samples,
                                   sample_t* out_samples,
                                   size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        size_t out_ch = 0;
        size_t in_ch = 0;

        while (out_ch < out_chans.num_channels()) {
            if (in_ch < in_chans.num_channels()) {
                *out_samples = *in_samples;
                in_samples++;
                in_ch++;
            } else {
                *out_samples = 0;
            }
            out_samples++;
            out_ch++;
        }

        if (in_ch < in_chans.num_channels()) {
            in_samples += (in_chans.num_channels() - in_ch);
        }
    }
}

// Layouts: multitrack => multitrack
// Algorithm:
//  - copy channels with indicies present in both output and input bitmasks
//  - zeroise output channels with indicies not present in input bitmask
//  - ignore input channels with indicies not present in output bitmask
void map_multitrack_to_multitrack(const ChannelSet& in_chans,
                                  const ChannelSet& out_chans,
                                  const ChannelSet& inout_chans,
                                  const sample_t* in_samples,
                                  sample_t* out_samples,
                                  size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        for (size_t ch = inout_chans.first_channel(); ch <= inout_chans.last_channel();
             ch++) {
            if (in_chans.has_channel(ch)) {
                if (out_chans.has_channel(ch)) {
                    *out_samples++ = *in_samples;
                }
                in_samples++;
            } else {
                if (out_chans.has_channel(ch)) {
                    *out_samples++ = 0;
                }
            }
        }
    }
}

} // namespace

ChannelMapper::ChannelMapper(const ChannelSet& in_chans, const ChannelSet& out_chans)
    : in_chans_(in_chans)
    , out_chans_(out_chans) {
    if (!in_chans_.is_valid()) {
        roc_panic("channel mapper: invalid input channel set: %s",
                  channel_set_to_str(in_chans).c_str());
    }

    if (!out_chans_.is_valid()) {
        roc_panic("channel mapper: invalid output channel set: %s",
                  channel_set_to_str(out_chans).c_str());
    }

    inout_chans_ = in_chans;
    inout_chans_.bitwise_or(out_chans);

    map_func_ = select_func_();
    if (!map_func_) {
        roc_panic("channel mapper: can't select mapper function");
    }
}

void ChannelMapper::map(const Frame& in_frame, Frame& out_frame) {
    if (in_frame.num_samples() % in_chans_.num_channels() != 0) {
        roc_panic("channel mapper: invalid input frame size:"
                  " in_frame_samples=%lu in_chans=%lu",
                  (unsigned long)in_frame.num_samples(),
                  (unsigned long)in_chans_.num_channels());
    }

    if (out_frame.num_samples() % out_chans_.num_channels() != 0) {
        roc_panic("channel mapper: invalid output frame size:"
                  " out_frame_samples=%lu out_chans=%lu",
                  (unsigned long)out_frame.num_samples(),
                  (unsigned long)out_chans_.num_channels());
    }

    if (in_frame.num_samples() / in_chans_.num_channels()
        != out_frame.num_samples() / out_chans_.num_channels()) {
        roc_panic("channel mapper: mismatching frame sizes:"
                  " in_frame_samples=%lu out_frame_samples=%lu",
                  (unsigned long)in_frame.num_samples(),
                  (unsigned long)out_frame.num_samples());
    }

    const size_t n_samples = in_frame.num_samples() / in_chans_.num_channels();

    const sample_t* in_samples = in_frame.samples();
    sample_t* out_samples = out_frame.samples();

    map_func_(in_chans_, out_chans_, inout_chans_, in_samples, out_samples, n_samples);
}

ChannelMapper::map_func_t ChannelMapper::select_func_() {
    switch (in_chans_.layout()) {
    case ChannelLayout_Invalid:
        return NULL;

    case ChannelLayout_Mono:
        switch (out_chans_.layout()) {
        case ChannelLayout_Invalid:
            return NULL;

        case ChannelLayout_Mono:
            return map_mono_to_mono;

        case ChannelLayout_Surround:
            return map_mono_to_surround;

        case ChannelLayout_Multitrack:
            return map_multitrack_non_multitrack;
        }
        return NULL;

    case ChannelLayout_Surround:
        switch (out_chans_.layout()) {
        case ChannelLayout_Invalid:
            return NULL;

        case ChannelLayout_Mono:
            return map_surround_to_mono;

        case ChannelLayout_Surround:
            return map_surround_to_surround;

        case ChannelLayout_Multitrack:
            return map_multitrack_non_multitrack;
        }
        return NULL;

    case ChannelLayout_Multitrack:
        switch (out_chans_.layout()) {
        case ChannelLayout_Invalid:
            return NULL;

        case ChannelLayout_Mono:
            return map_multitrack_non_multitrack;

        case ChannelLayout_Surround:
            return map_multitrack_non_multitrack;

        case ChannelLayout_Multitrack:
            return map_multitrack_to_multitrack;
        }
        return NULL;
    }

    return NULL;
}

} // namespace audio
} // namespace roc
