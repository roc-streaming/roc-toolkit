/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_RTP_TEST_PACKETS_PACKET_INFO_H_
#define ROC_RTP_TEST_PACKETS_PACKET_INFO_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace rtp {
namespace test {

struct PacketInfo {
    enum { MaxData = 2000, MaxExt = 32, MaxSrc = 16, MaxCh = 16, MaxSamples = 500 };

    uint8_t raw_data[MaxData];

    size_t packet_size;
    size_t header_size;
    size_t extension_size;
    size_t payload_size;
    size_t padding_size;

    uint8_t version;
    bool padding;
    bool extension;
    uint8_t num_csrc;
    uint8_t pt;
    bool marker;

    uint16_t seqnum;
    uint32_t ts;
    uint32_t ssrc;
    uint32_t csrc[MaxSrc];

    uint16_t ext_type;
    uint8_t ext_data_size;
    uint8_t ext_data[MaxExt];

    size_t num_channels;
    size_t num_samples;
    size_t samplebits;
    size_t samplerate;
    long samples[MaxCh][MaxSamples];
};

} // namespace test
} // namespace rtp
} // namespace roc

#endif // ROC_RTP_TEST_PACKETS_PACKET_INFO_H_
