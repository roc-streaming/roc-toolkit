/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/errno_to_str.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

errno_to_str::errno_to_str() {
    format_(GetLastError());
}

errno_to_str::errno_to_str(int err) {
    format_(err);
}

void errno_to_str::format_(int err) {
    wchar_t wbuf[sizeof(buffer_)] = {};

    size_t size =
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), wbuf,
                       sizeof(wbuf) / sizeof(wchar_t) - 1, nullptr);

    if (size == 0) {
        strcpy(buffer_, "<unknown error>");
        return;
    }

    while (size > 0 && (wbuf[size - 1] == L'\n' || wbuf[size - 1] == L'\r')) {
        wbuf[size - 1] = L'\0';
        size--;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buffer_, sizeof(buffer_), nullptr,
                            nullptr)
        == 0) {
        strcpy(buffer_, "<unknown error>");
    }
}

} // namespace core
} // namespace roc
