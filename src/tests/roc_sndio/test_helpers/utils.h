/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_SNDIO_TEST_HELPERS_UTILS_H_
#define ROC_SNDIO_TEST_HELPERS_UTILS_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/backend_map.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {
namespace test {
namespace {

bool backend_supports_format(IBackend& backend, core::IArena& arena, const char* format) {
    core::Array<FormatInfo, MaxFormats> format_list(arena);
    CHECK(backend.discover_formats(format_list));
    for (size_t n = 0; n < format_list.size(); n++) {
        if (strcmp(format_list[n].format_name, format) == 0) {
            return true;
        }
    }
    return false;
}

void expect_open_sink(status::StatusCode expected_code,
                      IBackend& backend,
                      audio::FrameFactory& frame_factory,
                      core::IArena& arena,
                      const char* driver,
                      const char* path,
                      const IoConfig& config,
                      core::ScopedPtr<ISink>& result) {
    IDevice* device = NULL;
    const status::StatusCode code = backend.open_device(
        DeviceType_Sink, driver, path, config, frame_factory, arena, &device);

    if (code != expected_code) {
        char buf[1024] = {};
        snprintf(buf, sizeof(buf),
                 "unexpected code when opening sink:"
                 "\n    backend:   %s\n    driver:    %s\n    path:      %s"
                 "\n    expected:  %s\n    actual:    %s\n",
                 backend.name(), driver ? driver : "<null>", path,
                 status::code_to_str(expected_code), status::code_to_str(code));
        FAIL(buf);
    }

    if (code == status::StatusOK) {
        CHECK(device);
        CHECK(device->to_sink());
        result.reset(device->to_sink());
    } else {
        CHECK(!device);
    }
}

void expect_open_source(status::StatusCode expected_code,
                        IBackend& backend,
                        audio::FrameFactory& frame_factory,
                        core::IArena& arena,
                        const char* driver,
                        const char* path,
                        const IoConfig& config,
                        core::ScopedPtr<ISource>& result) {
    IDevice* device = NULL;
    const status::StatusCode code = backend.open_device(
        DeviceType_Source, driver, path, config, frame_factory, arena, &device);

    if (code != expected_code) {
        char buf[1024] = {};
        snprintf(buf, sizeof(buf),
                 "unexpected code when opening source:"
                 "\n    backend:   %s\n    driver:    %s\n    path:      %s"
                 "\n    expected:  %s\n    actual:    %s\n",
                 backend.name(), driver ? driver : "<null>", path,
                 status::code_to_str(expected_code), status::code_to_str(code));
        FAIL(buf);
    }

    if (code == status::StatusOK) {
        CHECK(device);
        CHECK(device->to_source());
        result.reset(device->to_source());
    } else {
        CHECK(!device);
    }
}

void expect_specs_equal(const char* backend,
                        const audio::SampleSpec& expected,
                        const audio::SampleSpec& actual) {
    if (expected != actual) {
        char buf[1024] = {};
        snprintf(buf, sizeof(buf),
                 "unexpected sample spec:"
                 "\n    backend:   %s\n    expected:  %s\n    actual:    %s\n",
                 backend, audio::sample_spec_to_str(expected).c_str(),
                 audio::sample_spec_to_str(actual).c_str());
        FAIL(buf);
    }
}

} // namespace
} // namespace test
} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_TEST_HELPERS_UTILS_H_
