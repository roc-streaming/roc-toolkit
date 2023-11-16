/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample.h"

const roc::audio::PcmFormat roc::audio::SampleFormat(roc::audio::PcmCode_Float32,
                                                     roc::audio::PcmEndian_Native);

const roc::audio::sample_t roc::audio::SampleMin = -1;
const roc::audio::sample_t roc::audio::SampleMax = 1;
