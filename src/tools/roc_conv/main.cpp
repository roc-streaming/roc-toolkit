/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/null_writer.h"
#include "roc_audio/poison_writer.h"
#include "roc_audio/profiling_writer.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/scoped_destructor.h"
#include "roc_pipeline/config.h"
#include "roc_sndio/sox.h"
#include "roc_sndio/sox_reader.h"
#include "roc_sndio/sox_writer.h"

#include "roc_conv/cmdline.h"

using namespace roc;

namespace {

enum { MaxFrameSize = 8192, Channels = 0x3 };

} // namespace

int main(int argc, char** argv) {
    core::CrashHandler crash_handler;

    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::ScopedDestructor<gengetopt_args_info*, cmdline_parser_free>
        args_destructor(&args);

    core::Logger::instance().set_level(
        LogLevel(core::DefaultLogLevel + args.verbose_given));

    sndio::sox_setup();

    core::HeapAllocator allocator;
    core::BufferPool<audio::sample_t> pool(allocator, MaxFrameSize, args.poisoning_flag);

    size_t frame_size = pipeline::DefaultFrameSize;
    if (args.frame_size_given) {
        frame_size = (size_t)args.frame_size_arg;
    }

    sndio::SoxReader reader(pool, Channels, frame_size, 0);
    if (!reader.open(args.input_arg, NULL)) {
        roc_log(LogError, "can't open input file: %s", args.input_arg);
        return 1;
    }
    if (!reader.is_file()) {
        roc_log(LogError, "not a file: %s", args.input_arg);
        return 1;
    }

    size_t writer_sample_rate;
    if (args.rate_given) {
        writer_sample_rate = (size_t)args.rate_arg;
    } else {
        writer_sample_rate = reader.sample_rate();
    }

    sndio::SoxWriter output(allocator, Channels, writer_sample_rate);
    audio::NullWriter null;

    audio::IWriter* writer;
    if (args.output_given) {
        writer = &output;
    } else {
        roc_log(LogInfo, "no output given, using null output");
        writer = &null;
    }

    audio::PoisonWriter resampler_poisoner(*writer);
    if (args.poisoning_flag) {
        writer = &resampler_poisoner;
    }

    audio::ResamplerConfig resampler_config;
    switch ((unsigned)args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        resampler_config = audio::resampler_profile(audio::ResamplerProfile_Low);
        break;

    case resampler_profile_arg_medium:
        resampler_config = audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;

    case resampler_profile_arg_high:
        resampler_config = audio::resampler_profile(audio::ResamplerProfile_High);
        break;

    default:
        break;
    }
    if (args.resampler_interp_given) {
        resampler_config.window_interp = (size_t)args.resampler_interp_arg;
    }
    if (args.resampler_window_given) {
        resampler_config.window_size = (size_t)args.resampler_window_arg;
    }

    audio::ResamplerWriter resampler(*writer, pool, allocator, resampler_config, Channels,
                                     frame_size);
    if (!args.no_resampling_flag) {
        if (!resampler.valid()) {
            roc_log(LogError, "can't create resampler");
            return 1;
        }
        if (!resampler.set_scaling((float)reader.sample_rate() / writer_sample_rate)) {
            roc_log(LogError, "can't set resampler scaling");
            return 1;
        }
        writer = &resampler;
    }

    audio::ProfilingWriter profiler(*writer, Channels, reader.sample_rate());
    writer = &profiler;

    audio::PoisonWriter pipeline_poisoner(*writer);
    if (args.poisoning_flag) {
        writer = &pipeline_poisoner;
    }

    if (args.output_given) {
        if (!output.open(args.output_arg, NULL)) {
            roc_log(LogError, "can't open output file: %s", args.output_arg);
            return 1;
        }
        if (!output.is_file()) {
            roc_log(LogError, "not a file: %s", args.output_arg);
            return 1;
        }
    }

    int status = 1;

    if (reader.start(*writer)) {
        reader.join();
        status = 0;
        roc_log(LogInfo, "done");
    } else {
        roc_log(LogError, "can't start reader");
    }

    return status;
}
