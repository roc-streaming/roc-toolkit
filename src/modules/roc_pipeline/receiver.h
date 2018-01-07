/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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
#include "roc_core/buffer_pool.h"
#include "roc_core/cond.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/unique_ptr.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/ireceiver.h"
#include "roc_pipeline/receiver_port.h"
#include "roc_pipeline/receiver_session.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

//! Receiver pipeline.
class Receiver : public IReceiver, public packet::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    Receiver(const ReceiverConfig& config,
             const rtp::FormatMap& format_map,
             packet::PacketPool& packet_pool,
             core::BufferPool<uint8_t>& byte_buffer_pool,
             core::BufferPool<audio::sample_t>& sample_buffer_pool,
             core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Add receiving port.
    bool add_port(const PortConfig& config);

    //! Iterate added ports.
    void iterate_ports(void (*fn)(void*, const PortConfig&), void* arg) const;

    //! Get number of alive sessions.
    size_t num_sessions() const;

    //! Write packet.
    virtual void write(const packet::PacketPtr&);

    //! Read frame.
    virtual void read(audio::Frame&);

    //! Get current receiver status.
    virtual Status status() const;

    //! Wait until the receiver status becomes active.
    virtual void wait_active() const;

private:
    Status status_() const;

    void prepare_();

    void fetch_packets_();

    bool parse_packet_(const packet::PacketPtr& packet);
    bool route_packet_(const packet::PacketPtr& packet);

    bool create_session_(const packet::PacketPtr& packet);
    void remove_session_(ReceiverSession& sess);

    void update_sessions_();

    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;
    core::IAllocator& allocator_;

    core::List<ReceiverPort> ports_;
    core::List<ReceiverSession> sessions_;

    core::List<packet::Packet> packets_;

    audio::Mixer mixer_;
    core::Ticker ticker_;

    ReceiverConfig config_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;

    bool valid_;

    core::Mutex control_mutex_;
    core::Mutex pipeline_mutex_;
    core::Cond active_cond_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_H_
