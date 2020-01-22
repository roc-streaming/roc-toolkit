/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/protocol_map.h
//! @brief Protocol attributes map.

#ifndef ROC_ADDRESS_PROTOCOL_MAP_H_
#define ROC_ADDRESS_PROTOCOL_MAP_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"
#include "roc_core/stddefs.h"
#include "roc_packet/fec.h"

namespace roc {
namespace address {

//! Protocol attributes.
struct ProtocolAttrs {
    //! Protocol ID.
    EndpointProtocol protocol;

    //! Endpoint type.
    EndpointType type;

    //! FEC scheme associated wit hthe protocol, if any.
    packet::FecScheme fec_scheme;

    //! Default port number of -1 if not specified.
    int default_port;

    //! Whether path is supported in URI.
    bool path_supported;

    ProtocolAttrs()
        : protocol(EndProto_None)
        , type(EndType_Session)
        , fec_scheme(packet::FEC_None)
        , default_port(-1)
        , path_supported(false) {
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
    const ProtocolAttrs* find_proto(EndpointProtocol proto) const;

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
