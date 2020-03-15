/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sdp/session_description.h"

namespace roc {
namespace sdp {

SessionDescription::SessionDescription(core::IAllocator& allocator)
    : guid_(allocator)
    , media_descriptions_()
    , allocator_(allocator) {
    clear();
}

void SessionDescription::clear() {
    guid_.clear();
    origin_unicast_address_.clear();
    session_connection_address_.clear();

    while(media_descriptions_.size() > 0) {
        core::SharedPtr<MediaDescription> m = media_descriptions_.back();
        media_descriptions_.remove(*m);
    }
}

const char* SessionDescription::guid() const {
    if (guid_.is_empty()) {
        return NULL;
    }
    return guid_.c_str();
}

bool SessionDescription::set_guid(const char* start_p_origin_username,
                                  const char* end_p_origin_sess_id,
                                  const char* start_p_origin_nettype,
                                  const char* end_p_origin_addr) {
    core::StringBuilder b(guid_.raw_buf());

    if (!b.append_str_range(start_p_origin_username, end_p_origin_sess_id))
        return false;

    b.append_char(' ');

    if (!b.append_str_range(start_p_origin_nettype, end_p_origin_addr))
        return false;

    roc_log(LogInfo, "parsed guid: %s", guid_.c_str());

    return true;
}

address::AddrFamily SessionDescription::origin_addrtype() const {
    return origin_addrtype_;
}

bool SessionDescription::set_origin_addrtype(address::AddrFamily addrtype) {
    origin_addrtype_ = addrtype;
    return true;
}

const address::SocketAddr SessionDescription::origin_unicast_address() const {
    return origin_unicast_address_;
}

bool SessionDescription::set_origin_unicast_address(const char* str, size_t str_len) {
    char addr[address::SocketAddr::MaxStrLen];
    core::StringBuilder b(addr, sizeof(addr));
    if (!b.append_str_range(str, str + str_len))
        return false;

    roc_log(LogInfo, "Unicast address: %s", &addr[0]);

    if (!origin_unicast_address_.set_host_port(origin_addrtype_, addr, 0))
        return false;

    return true;
}

bool SessionDescription::set_session_connection_address(address::AddrFamily addrtype,
                                                        const char* str,
                                                        size_t str_len) {
    char addr[address::SocketAddr::MaxStrLen];
    core::StringBuilder b(addr, sizeof(addr));

    if (!b.append_str_range(str, str + str_len))
        return false;

    roc_log(LogInfo, "Connection address: %s", &addr[0]);

    if (!session_connection_address_.set_host_port(addrtype, addr, 0))
        return false;

    return true;
}

bool SessionDescription::add_media_description(const char* str, size_t str_len) {
    core::SharedPtr<MediaDescription> media =
        new (allocator_) MediaDescription(allocator_);

    if (!media->set_media(str, str_len)) {
        return false;
    }

    media_descriptions_.push_back(*media);
    return true;
}

const core::SharedPtr<MediaDescription>
SessionDescription::last_media_description() const {
    return media_descriptions_.back();
}

bool SessionDescription::add_connection_to_last_media(address::AddrFamily addrtype,
                                                      const char* str,
                                                      size_t str_len) {

    core::SharedPtr<MediaDescription> m = media_descriptions_.back();
    return m->add_connection_field(addrtype, str, str_len);
    
}

} // namespace roc
} // namespace sdp
