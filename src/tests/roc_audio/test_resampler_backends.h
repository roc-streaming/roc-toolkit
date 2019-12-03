/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_RESAMPLER_BACKENDS_H_
#define ROC_AUDIO_TEST_RESAMPLER_BACKENDS_H_

#include "roc_audio/resampler_config.h"
#include "roc_core/helpers.h"

namespace roc {
namespace audio {

namespace {

ResamplerBackend Test_resampler_backends[] = { ResamplerBackend_Builtin };

const size_t Test_n_resampler_backends = ROC_ARRAY_SIZE(Test_resampler_backends);

} // namespace

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_RESAMPLER_BACKENDS_H_
