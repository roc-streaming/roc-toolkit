/*
 * Copyright (c) 2022 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/terminal.h"

namespace roc {
namespace sndio {

ITerminal::~ITerminal() {
}

const char* terminal_type_to_str(TerminalType type) {
    switch (type) {
    case Terminal_Sink:
        return "sink";

    case Terminal_Source:
        return "source";

    default:
        break;
    }

    return "<invalid>";
}

} // namespace sndio
} // namespace roc
