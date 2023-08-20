/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/protocol.h"
#include "roc_address/protocol_map.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

const char* proto_to_str(Protocol proto) {
    const ProtocolAttrs* attrs = ProtocolMap::instance().find_by_id(proto);
    if (!attrs) {
        return NULL;
    }

    return attrs->scheme_name;
}

} // namespace address
} // namespace roc
