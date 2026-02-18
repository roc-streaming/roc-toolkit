/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_status/code_to_str.h"
#include "roc_core/panic.h"
#include "roc_status/status_code.h"

namespace roc {
namespace status {

const char* code_to_str(StatusCode code) {
    switch (code) {
    case NoStatus:
    case MaxStatus:
        break;
    case StatusOK:
        return "OK";
    case StatusPart:
        return "Part";
    case StatusDrain:
        return "Drain";
    case StatusAbort:
        return "Abort";
    case StatusFinish:
        return "Finish";
    case StatusNoMem:
        return "NoMem";
    case StatusNoRoute:
        return "NoRoute";
    case StatusNoDriver:
        return "NoDriver";
    case StatusNoFormat:
        return "NoFormat";
    case StatusNoPlugin:
        return "NoPlugin";
    case StatusErrDevice:
        return "ErrDevice";
    case StatusErrFile:
        return "ErrFile";
    case StatusErrNetwork:
        return "ErrNetwork";
    case StatusErrThread:
        return "ErrThread";
    case StatusErrRand:
        return "ErrRand";
    case StatusBadSlot:
        return "BadSlot";
    case StatusBadInterface:
        return "BadInterface";
    case StatusBadProtocol:
        return "BadProtocol";
    case StatusBadConfig:
        return "BadConfig";
    case StatusBadPacket:
        return "BadPacket";
    case StatusBadBuffer:
        return "BadBuffer";
    case StatusBadArg:
        return "BadArg";
    case StatusBadOperation:
        return "BadOperation";
    case StatusBadState:
        return "BadState";
    }

    // Most likely someone forgot to initialize status to a proper
    // value and returned it.
    roc_panic("status: corrupted or uninitialized value: code=%d", code);
}

} // namespace status
} // namespace roc
