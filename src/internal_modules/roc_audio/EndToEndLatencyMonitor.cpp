//
// Created by misha on 31.08.23.
//

#include "EndToEndLatencyMonitor.h"

namespace roc {
namespace audio {

EndToEndLatencyMonitor::EndToEndLatencyMonitor(IFrameReader& reader)
    : reader_(reader)
    , valid_(false)
    , e2e_latency_(0) {

}

EndToEndLatencyMonitor::~EndToEndLatencyMonitor()
{}

bool EndToEndLatencyMonitor::is_valid() const
{
    return valid_;
}

bool EndToEndLatencyMonitor::read(Frame& frame)
{
    const bool res = reader_.read(frame);
    if (!!frame.capture_timestamp()){
        const core::nanoseconds_t cur_ts = core::timestamp(core::ClockMonotonic);
        valid_ = true;
        e2e_latency_ = cur_ts - frame.capture_timestamp();
    } else {
        valid_ = false;
    }

    return res;
}

core::nanoseconds_t EndToEndLatencyMonitor::latency() const {
    return e2e_latency_;
}

} // namespace audio
} // namespace roc
