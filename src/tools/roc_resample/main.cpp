/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_sndio/sox.h"
#include "roc_sndio/sox_reader.h"
#include "roc_sndio/sox_writer.h"

#include "roc_resample/cmdline.h"

using namespace roc;

namespace {

enum { MaxFrameSize = 65 * 1024 };

} // namespace

int main(int argc, char** argv) {
    core::CrashHandler crash_handler;

    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::Logger::instance().set_level(
        LogLevel(core::DefaultLogLevel + args.verbose_given));

    sndio::sox_setup();

    audio::ResamplerConfig resampler_config;

    packet::channel_mask_t channel_mask = 0x3;
    size_t sample_rate = (size_t)args.rate_arg;
    size_t chunk_size = 4096;

    core::HeapAllocator allocator;
    core::BufferPool<audio::sample_t> pool(allocator, MaxFrameSize, 1);

    sndio::SoxReader reader(pool, channel_mask, chunk_size, 0);

    if (!reader.open(args.input_arg, NULL)) {
        roc_log(LogError, "can't open input file: %s", args.input_arg);
        return 1;
    }

    if (!reader.is_file()) {
        roc_log(LogError, "not a file file: %s", args.input_arg);
        return 1;
    }

    if (!args.rate_given) {
        sample_rate = reader.sample_rate();
    }

    sndio::SoxWriter writer(allocator, channel_mask, sample_rate);

    if (!writer.open(args.output_arg, NULL)) {
        roc_log(LogError, "can't open output file: %s", args.output_arg);
        return 1;
    }

    if (!writer.is_file()) {
        roc_log(LogError, "not a file file: %s", args.output_arg);
        return 1;
    }

    audio::ResamplerWriter resampler(writer, pool, allocator, resampler_config,
                                     channel_mask);
    if (!resampler.valid()) {
        roc_log(LogError, "can't create resampler");
        return 1;
    }

    if (!resampler.set_scaling((float)reader.sample_rate() / sample_rate)) {
        roc_log(LogError, "can't set resampler scaling");
        return 1;
    }

    int status = 1;

    if (reader.start(resampler)) {
        reader.join();
        status = 0;
    } else {
        roc_log(LogError, "can't start reader");
    }

    return status;
}
