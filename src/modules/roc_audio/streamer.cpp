/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_config/config.h"

#include "roc_core/stddefs.h"
#include "roc_core/helpers.h"
#include "roc_core/math.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#include "roc_audio/streamer.h"

#define TS_IS_BEFORE(a, b) ROC_IS_BEFORE(packet::signed_timestamp_t, a, b)
#define TS_SUBTRACT(a, b) ROC_SUBTRACT(packet::signed_timestamp_t, a, b)

namespace roc {
namespace audio {

using packet::sample_t;
using packet::timestamp_t;

namespace {

enum { ReportInterval = 20000 /* ms */ };

inline void write_zeros(sample_t* buf, size_t bufsz) {
    memset(buf, 0, bufsz * sizeof(sample_t));
}

inline void write_beep(sample_t* buf, size_t bufsz) {
    for (size_t n = 0; n < bufsz; n++) {
        buf[n] = (sample_t)sin(2 * M_PI / ROC_CONFIG_DEFAULT_SAMPLE_RATE * 880 * n);
    }
}

} // namespace

Streamer::Streamer(packet::IPacketReader& reader, packet::channel_t channel, bool beep)
    : reader_(reader)
    , channel_(channel)
    , packet_pos_(0)
    , timestamp_(0)
    , zero_samples_(0)
    , missing_samples_(0)
    , packet_samples_(0)
    , timer_(ReportInterval)
    , first_packet_(true)
    , beep_(beep) {
}

void Streamer::read(const ISampleBufferSlice& buffer) {
    roc_panic_if(buffer.data() == NULL);

    sample_t* buff_ptr = buffer.data();
    sample_t* buff_end = buffer.data() + buffer.size();

    while (buff_ptr < buff_end) {
        buff_ptr = read_samples_(buff_ptr, buff_end);
    }

    roc_panic_if(buff_ptr != buff_end);

    if (timer_.expired()) {
        roc_log(LogDebug, "streamer: ch=%d ts=%lu loss_ratio=%.5lf", (int)channel_,
                (unsigned long)timestamp_,
                double(missing_samples_) / (missing_samples_ + packet_samples_));
    }
}

sample_t* Streamer::read_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    update_packet_();

    if (packet_) {
        timestamp_t next_timestamp = (packet_->timestamp() + packet_pos_);

        if (timestamp_ != next_timestamp) {
            roc_panic_if_not(TS_IS_BEFORE(timestamp_, next_timestamp));

            size_t mis_samples = (size_t)TS_SUBTRACT(next_timestamp, timestamp_);
            size_t max_samples = (size_t)(buff_end - buff_ptr);

            buff_ptr = read_missing_samples_(buff_ptr, buff_ptr
                                                 + ROC_MIN(mis_samples, max_samples));
        }

        if (buff_ptr < buff_end) {
            buff_ptr = read_packet_samples_(buff_ptr, buff_end);
        }

        return buff_ptr;
    } else {
        return read_missing_samples_(buff_ptr, buff_end);
    }
}

sample_t* Streamer::read_packet_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t pkt_samples = (size_t)(packet_->num_samples() - packet_pos_);
    const size_t max_samples = (size_t)(buff_end - buff_ptr);

    const size_t num_samples = ROC_MIN(pkt_samples, max_samples);

    const size_t ret =
        packet_->read_samples((1 << channel_), packet_pos_, buff_ptr, num_samples);

    if (ret != num_samples) {
        packet_->print(true);
        roc_panic("streamer: unexpected # of samples from packet:"
                  " ret=%lu ns=%lu pos=%lu",
                  (unsigned long)ret,         //
                  (unsigned long)num_samples, //
                  (unsigned long)packet_pos_);
    }

    timestamp_ += timestamp_t(num_samples);
    packet_pos_ += timestamp_t(num_samples);
    packet_samples_ += num_samples;

    if (packet_pos_ == packet_->num_samples()) {
        packet_.reset();
    }

    return (buff_ptr + num_samples);
}

sample_t* Streamer::read_missing_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    size_t num_samples = (size_t)(buff_end - buff_ptr);

    if (beep_) {
        write_beep(buff_ptr, num_samples);
    } else {
        write_zeros(buff_ptr, num_samples);
    }

    timestamp_ += timestamp_t(num_samples);

    if (first_packet_) {
        zero_samples_ += num_samples;
    } else {
        missing_samples_ += num_samples;
    }

    return (buff_ptr + num_samples);
}

void Streamer::update_packet_() {
    if (packet_) {
        return;
    }

    timestamp_t pkt_timestamp = 0;
    unsigned n_dropped = 0;

    while ((packet_ = read_packet_())) {
        pkt_timestamp = packet_->timestamp();

        if (first_packet_) {
            break;
        }

        if (TS_IS_BEFORE(timestamp_, pkt_timestamp + packet_->num_samples())) {
            break;
        }

        roc_log(LogDebug, "streamer: dropping late packet:"
                           " ch=%d ts=%lu pkt_ts=%lu pkt_ns=%lu",
                (int)channel_, (unsigned long)timestamp_, (unsigned long)pkt_timestamp,
                (unsigned long)packet_->num_samples());

        n_dropped++;
    }

    if (n_dropped != 0) {
        roc_log(LogInfo, "streamer: ch=%d fetched=%d dropped=%u", (int)channel_,
                (int)!!packet_, n_dropped);
    }

    if (!packet_) {
        return;
    }

    if (first_packet_) {
        roc_log(LogDebug, "streamer: got first packet: ch=%d zero_samples=%lu",
                (int)channel_, (unsigned long)zero_samples_);

        timestamp_ = pkt_timestamp;
        first_packet_ = false;
    }

    if (TS_IS_BEFORE(pkt_timestamp, timestamp_)) {
        packet_pos_ = (timestamp_t)TS_SUBTRACT(timestamp_, pkt_timestamp);
    } else {
        packet_pos_ = 0;
    }
}

packet::IAudioPacketConstPtr Streamer::read_packet_() {
    packet::IPacketConstPtr pp = reader_.read();
    if (!pp) {
        return NULL;
    }

    if (pp->type() != packet::IAudioPacket::Type) {
        roc_panic("streamer: got unexpected non-audio packet from reader");
    }

    return static_cast<const packet::IAudioPacket*>(pp.get());
}

} // namespace audio
} // namespace roc
