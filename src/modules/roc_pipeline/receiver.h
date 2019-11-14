/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver.h
//! @brief Receiver pipeline.

#ifndef ROC_PIPELINE_RECEIVER_H_
#define ROC_PIPELINE_RECEIVER_H_

#include "roc_audio/ireader.h"
#include "roc_audio/mixer.h"
#include "roc_audio/poison_reader.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/cond.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/unique_ptr.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_port.h"
#include "roc_pipeline/receiver_session.h"
#include "roc_rtp/format_map.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Receiver pipeline.
class Receiver : public sndio::ISource,
                 public packet::IWriter,
                 public core::NonCopyable<> {
public:
    //! Initialize.
    Receiver(const ReceiverConfig& config,
             const fec::CodecMap& codec_map,
             const rtp::FormatMap& format_map,
             packet::PacketPool& packet_pool,
             core::BufferPool<uint8_t>& byte_buffer_pool,
             core::BufferPool<audio::sample_t>& sample_buffer_pool,
             core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Add receiving port.
    bool add_port(const PortConfig& config);

    //! Get number of alive sessions.
    size_t num_sessions() const;

    //! Get current receiver state.
    virtual State state() const;

    //! Wait until the receiver status becomes active.
    virtual void wait_active() const;

    //! Get source sample rate.
    virtual size_t sample_rate() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Write packet.
    virtual void write(const packet::PacketPtr&);

    //! Read frame.
    virtual bool read(audio::Frame&);

private:
    State state_() const;

    void prepare_();

    void fetch_packets_();

    bool parse_packet_(const packet::PacketPtr& packet);
    bool route_packet_(const packet::PacketPtr& packet);

    bool can_create_session_(const packet::PacketPtr& packet);

    bool create_session_(const packet::PacketPtr& packet);
    void remove_session_(ReceiverSession& sess);

    void update_sessions_();

    ReceiverSessionConfig make_session_config_(const packet::PacketPtr& packet) const;

    const fec::CodecMap& codec_map_;
    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;
    core::IAllocator& allocator_;

    core::List<ReceiverPort> ports_;
    core::List<ReceiverSession> sessions_;

    core::List<packet::Packet> packets_;

    core::Ticker ticker_;

    core::UniquePtr<audio::Mixer> mixer_;
    core::UniquePtr<audio::PoisonReader> poisoner_;

    audio::IReader* audio_reader_;

    ReceiverConfig config_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;

    core::Mutex control_mutex_;
    core::Mutex pipeline_mutex_;
    core::Cond active_cond_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_H_
