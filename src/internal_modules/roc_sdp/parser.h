/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/parser.h
//! @brief Parser for the Session Description Protocol.

#ifndef ROC_SDP_PARSER_H_
#define ROC_SDP_PARSER_H_

#include "roc_sdp/session_description.h"

namespace roc {
namespace sdp {

//! Parse SDP session description from string.
bool parse_sdp(const char* str, SessionDescription& result);

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_PARSER_H_
