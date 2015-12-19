/*
 * THIS FILE IS AUTO-GENERATED USING `generate_headers.py'
 *
 * Input:
 *  - rtp_l16_1ch_10s_12ext.json
 *  - rtp_l16_1ch_10s_12ext.blob
 */

#ifndef ROC_RTP_TEST_BLOBS_RTP_L16_1CH_10S_12EXT_H_
#define ROC_RTP_TEST_BLOBS_RTP_L16_1CH_10S_12EXT_H_

#include "test_rtp_packet.h"

namespace roc {
namespace test {

static RTP_PacketTest rtp_l16_1ch_10s_12ext = {
  /* raw_data         */ {
    0x90, 0x0b, 0xd4, 0x31, 0x07, 0x5b, 0xcd, 0x15, 0x00, 0x00, 0x01, 0xf4,
    0x00, 0x05, 0x00, 0x02, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05,
    0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x09,
  },

  /* packet_size      */ 44,
  /* header_size      */ 12,
  /* extension_size   */ 12,
  /* payload_size     */ 20,
  /* padding_size     */ 0,

  /* version          */ 2,
  /* padding          */ 0,
  /* extension        */ 1,
  /* num_csrc         */ 0,
  /* pt               */ 11,
  /* marker           */ 0,

  /* seqnum           */ 54321,
  /* ts               */ 123456789,
  /* ssrc             */ 500,
  /* csrc             */ {  },

  /* ext_type         */ 5,
  /* ext_data_size    */ 8,
  /* ext_data         */ { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68 },

  /* num_channels     */ 1,
  /* num_samples      */ 10,
  /* samplebits       */ 16,

  /* samples          */ {
    /* channel #0       */ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
  },
};

} // namespace test
} // namespace roc

#endif // ROC_RTP_TEST_BLOBS_RTP_L16_1CH_10S_12EXT_H_
