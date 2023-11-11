/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/protocol_map.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

ProtocolMap::ProtocolMap() {
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTSP;
        attrs.iface = Iface_Consolidated;
        attrs.scheme_name = "rtsp";
        attrs.path_supported = true;
        attrs.default_port = 554;
        attrs.fec_scheme = packet::FEC_None;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTP;
        attrs.iface = Iface_AudioSource;
        attrs.scheme_name = "rtp";
        attrs.path_supported = false;
        attrs.default_port = -1;
        attrs.fec_scheme = packet::FEC_None;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTP_RS8M_Source;
        attrs.iface = Iface_AudioSource;
        attrs.scheme_name = "rtp+rs8m";
        attrs.path_supported = false;
        attrs.default_port = -1;
        attrs.fec_scheme = packet::FEC_ReedSolomon_M8;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RS8M_Repair;
        attrs.iface = Iface_AudioRepair;
        attrs.scheme_name = "rs8m";
        attrs.path_supported = false;
        attrs.default_port = -1;
        attrs.fec_scheme = packet::FEC_ReedSolomon_M8;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTP_LDPC_Source;
        attrs.iface = Iface_AudioSource;
        attrs.scheme_name = "rtp+ldpc";
        attrs.path_supported = false;
        attrs.default_port = -1;
        attrs.fec_scheme = packet::FEC_LDPC_Staircase;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_LDPC_Repair;
        attrs.iface = Iface_AudioRepair;
        attrs.scheme_name = "ldpc";
        attrs.path_supported = false;
        attrs.default_port = -1;
        attrs.fec_scheme = packet::FEC_LDPC_Staircase;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTCP;
        attrs.iface = Iface_AudioControl;
        attrs.scheme_name = "rtcp";
        attrs.path_supported = false;
        attrs.default_port = -1;
        attrs.fec_scheme = packet::FEC_None;
        add_proto_(attrs);
    }
}

const ProtocolAttrs* ProtocolMap::find_by_id(Protocol proto) const {
    if ((int)proto < 0 || (int)proto >= MaxProtos) {
        return NULL;
    }

    if (protos_[proto].protocol == Proto_None) {
        return NULL;
    }

    if (protos_[proto].protocol != proto) {
        return NULL;
    }

    return &protos_[proto];
}

const ProtocolAttrs* ProtocolMap::find_by_scheme(const char* scheme) const {
    for (int proto = 0; proto < MaxProtos; proto++) {
        if (protos_[proto].protocol == Proto_None) {
            continue;
        }

        if (protos_[proto].scheme_name == NULL) {
            continue;
        }

        if (strcmp(protos_[proto].scheme_name, scheme) != 0) {
            continue;
        }

        return &protos_[proto];
    }

    return NULL;
}

void ProtocolMap::add_proto_(const ProtocolAttrs& proto) {
    roc_panic_if((int)proto.protocol < 0);
    roc_panic_if((int)proto.protocol >= MaxProtos);
    roc_panic_if(protos_[proto.protocol].protocol != 0);

    protos_[proto.protocol] = proto;
}

bool ProtocolMap::get_supported_interfaces(core::Array<Interface>& interface_array) {
    interface_array.clear();
    bool interfaces_exist = false;

    for (size_t x = Iface_Consolidated + 1; x != Iface_Max; x++) {
        for (size_t y = 0; y < MaxProtos; y++) {
            if (x == protos_[y].iface) {
                interface_array.push_back(protos_[y].iface);
                interfaces_exist = true;
                break;
            }
        }
    }

    return interfaces_exist;
}

bool ProtocolMap::get_supported_protocols(Interface interface, core::StringList& list) {
    list.clear();

    bool protocolsExist = false;

    for (size_t x = 0; x < MaxProtos; x++) {
        if (interface == ProtocolMap::instance().protos_[x].iface) {
            if (!list.push_unique(
                    proto_to_str(ProtocolMap::instance().protos_[x].protocol))) {
                return false;
            }
            protocolsExist = true;
        }
    }

    return protocolsExist;
}

} // namespace address
} // namespace roc
