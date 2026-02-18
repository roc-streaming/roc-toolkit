/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample.h"

const roc::audio::PcmSubformat roc::audio::PcmSubformat_Raw =
    roc::audio::PcmSubformat_Float32;

const roc::audio::sample_t roc::audio::Sample_Min = -1;
const roc::audio::sample_t roc::audio::Sample_Max = 1;
