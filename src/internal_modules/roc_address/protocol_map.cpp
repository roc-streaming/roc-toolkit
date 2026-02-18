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
        attrs.iface = Iface_Aggregate;
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

bool ProtocolMap::get_supported_interfaces(core::Array<Interface>& interface_array) {
    interface_array.clear();
    bool interfaces_exist = false;

    for (unsigned n_iface = (unsigned)Iface_Aggregate; n_iface != (unsigned)Iface_Max;
         n_iface++) {
        for (size_t n_proto = 0; n_proto < MaxProtos; n_proto++) {
            if (protos_[n_proto].protocol == Proto_None) {
                continue;
            }

            if (n_iface == (unsigned)protos_[n_proto].iface) {
                if (!interface_array.push_back(protos_[n_proto].iface)) {
                    return false;
                }
                interfaces_exist = true;
                break;
            }
        }
    }

    return interfaces_exist;
}

bool ProtocolMap::get_supported_protocols(Interface interface, core::StringList& list) {
    list.clear();

    bool protocols_exist = false;

    for (size_t n_proto = 0; n_proto < MaxProtos; n_proto++) {
        if (protos_[n_proto].protocol == Proto_None) {
            continue;
        }

        if (interface == ProtocolMap::instance().protos_[n_proto].iface) {
            const char* proto_name = protos_[n_proto].scheme_name;

            if (!list.find(proto_name)) {
                if (!list.push_back(proto_name)) {
                    return false;
                }
            }
            protocols_exist = true;
        }
    }

    return protocols_exist;
}

void ProtocolMap::add_proto_(const ProtocolAttrs& proto) {
    roc_panic_if((int)proto.protocol < 0);
    roc_panic_if((int)proto.protocol >= MaxProtos);
    roc_panic_if(protos_[proto.protocol].protocol != 0);

    protos_[proto.protocol] = proto;
}

} // namespace address
} // namespace roc
