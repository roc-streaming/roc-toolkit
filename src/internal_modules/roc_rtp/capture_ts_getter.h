/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_RTP_CAPTURE_TS_GETTER_H_
#define ROC_RTP_CAPTURE_TS_GETTER_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace rtp {

//! Remembers a recent pair of capture timestamp and rtp ts.
class CaptureTsGetter : public packet::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    CaptureTsGetter(packet::IWriter& writer);
    //! Destroy.
    virtual ~CaptureTsGetter();

    //! Passes pkt downstream and remembers its capture and rtp timestamps.
    virtual void write(const packet::PacketPtr& pkt);

    //! Get the last remembers its capture and rtp timestamps.
    bool get(core::nanoseconds_t& ns, packet::timestamp_t& rtp) const;

private:
    packet::IWriter& writer_;

    bool valid_;
    core::nanoseconds_t capt_ts_;
    packet::timestamp_t rtp_ts_;

};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_CAPTURE_TS_GETTER_H_
