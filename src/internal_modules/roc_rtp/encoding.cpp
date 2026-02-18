/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/encoding.h"
#include "roc_audio/sample_spec.h"

namespace roc {
namespace rtp {

bool parse_encoding(const char* str, Encoding& result) {
    result = Encoding();

    if (str == NULL) {
        roc_log(LogError, "parse encoding: string is null");
        return false;
    }

    const char* sep = strchr(str, ':');
    if (sep == NULL) {
        roc_log(LogError,
                "parse encoding: invalid format: missing separator, expected"
                " '<id>:<spec>', got '%s'",
                str);
        return false;
    }

    if (!isdigit(*str)) {
        roc_log(LogError,
                "parse encoding: invalid id: not a number, expected"
                " '<id>:<spec>', got '%s'",
                str);
        return false;
    }

    char* number_end = NULL;
    const unsigned long number = strtoul(str, &number_end, 10);

    if (number == ULONG_MAX || !number_end || number_end != sep) {
        roc_log(LogError,
                "parse encoding: invalid id: not a number, expected"
                " '<id>:<spec>', got '%s'",
                str);
        return false;
    }

    if (number > UINT_MAX) {
        roc_log(LogError,
                "parse encoding: invalid id: out of range:"
                " got=%lu max=%u",
                number, UINT_MAX);
        return false;
    }

    if (!audio::parse_sample_spec(sep + 1, result.sample_spec)) {
        roc_log(LogError, "parse encoding: invalid spec");
        return false;
    }

    result.payload_type = (unsigned int)number;
    result.packet_flags = packet::Packet::FlagAudio;

    return true;
}

} // namespace rtp
} // namespace roc
