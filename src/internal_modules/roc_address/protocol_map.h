/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/protocol_map.h
//! @brief Protocol attributes map.

#ifndef ROC_ADDRESS_PROTOCOL_MAP_H_
#define ROC_ADDRESS_PROTOCOL_MAP_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_list.h"
#include "roc_packet/fec.h"

namespace roc {
namespace address {

//! Protocol attributes.
struct ProtocolAttrs {
    //! Protocol ID.
    Protocol protocol;

    //! Endpoint type.
    Interface iface;

    //! Scheme name in URI.
    const char* scheme_name;

    //! Whether path is supported in URI.
    bool path_supported;

    //! Default port number of -1 if not specified.
    int default_port;

    //! FEC scheme associated wit the protocol, if any.
    packet::FecScheme fec_scheme;

    ProtocolAttrs()
        : protocol(Proto_None)
        , iface(Iface_Invalid)
        , scheme_name(NULL)
        , path_supported(false)
        , default_port(-1)
        , fec_scheme(packet::FEC_None) {
    }
};

//! Protocol attributes map.
class ProtocolMap : public core::NonCopyable<> {
public:
    //! Get instance.
    static ProtocolMap& instance() {
        return core::Singleton<ProtocolMap>::instance();
    }

    //! Get protocol attributes by ID.
    const ProtocolAttrs* find_by_id(Protocol proto) const;

    //! Get protocol attributes by scheme name.
    const ProtocolAttrs* find_by_scheme(const char* scheme) const;

    //! Get list of interfaces with at least one protocol
    ROC_ATTR_NODISCARD bool get_supported_interfaces(core::Array<Interface>&);

    //! Get all supported protocols
    ROC_ATTR_NODISCARD bool get_supported_protocols(Interface, core::StringList&);

private:
    friend class core::Singleton<ProtocolMap>;

    enum { MaxProtos = 8 };

    ProtocolMap();

    void add_proto_(const ProtocolAttrs&);

    ProtocolAttrs protos_[MaxProtos];
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_PROTOCOL_MAP_H_
