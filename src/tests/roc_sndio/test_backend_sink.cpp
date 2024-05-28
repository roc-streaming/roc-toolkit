/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/pump.h"

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 500, SampleRate = 48000 };

const audio::SampleSpec sample_spec(SampleRate,
                                    audio::Sample_RawFormat,
                                    audio::ChanLayout_Surround,
                                    audio::ChanOrder_Smpte,
                                    audio::ChanMask_Surround_Stereo);

const core::nanoseconds_t frame_duration = FrameSize * core::Second
    / core::nanoseconds_t(sample_spec.sample_rate() * sample_spec.num_channels());

core::HeapArena arena;
audio::FrameFactory frame_factory(arena, FrameSize * sizeof(audio::sample_t));

} // namespace

TEST_GROUP(backend_sink) {
    Config sink_config;

    void setup() {
        sink_config.sample_spec = sample_spec;
        sink_config.frame_length = frame_duration;
    }
};

TEST(backend_sink, open) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        core::TempFile file("test.wav");

        core::ScopedPtr<ISink> backend_sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                               DriverType_File, NULL, file.path(), sink_config,
                               backend_sink);

        test::expect_specs_equal(backend.name(), sink_config.sample_spec,
                                 backend_sink->sample_spec());

        CHECK(!backend_sink->has_state());
        CHECK(!backend_sink->has_latency());
        CHECK(!backend_sink->has_clock());
    }
}

// Open fails because file doesn't exist.
TEST(backend_sink, open_bad_file) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        core::ScopedPtr<ISink> backend_sink;
        test::expect_open_sink(status::StatusErrFile, backend, frame_factory, arena,
                               DriverType_File, NULL, "/bad/file.wav", sink_config,
                               backend_sink);
    }
}

// Open fails because of invalid sndio::Config.
TEST(backend_sink, open_bad_config) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        core::TempFile file("test.wav");

        Config bad_config = sink_config;
        bad_config.sample_spec.set_pcm_format(audio::PcmFormat_SInt18_3_Be);

        core::ScopedPtr<ISink> backend_sink;
        test::expect_open_sink(status::StatusBadConfig, backend, frame_factory, arena,
                               DriverType_File, NULL, file.path(), bad_config,
                               backend_sink);
    }
}

// If config is empty, open uses default values.
TEST(backend_sink, open_default_config) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        core::TempFile file("test.wav");

        Config default_config = sink_config;
        default_config.sample_spec.clear();

        core::ScopedPtr<ISink> backend_sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                               DriverType_File, NULL, file.path(), default_config,
                               backend_sink);

        CHECK(backend_sink->sample_spec().is_valid());
    }
}

} // namespace sndio
} // namespace roc
