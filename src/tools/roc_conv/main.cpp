/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_profile.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/scoped_destructor.h"
#include "roc_pipeline/converter.h"
#include "roc_sndio/pump.h"
#include "roc_sndio/sox_controller.h"
#include "roc_sndio/sox_sink.h"
#include "roc_sndio/sox_source.h"

#include "roc_conv/cmdline.h"

using namespace roc;

int main(int argc, char** argv) {
    core::CrashHandler crash_handler;

    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::ScopedDestructor<gengetopt_args_info*, cmdline_parser_free> args_destructor(
        &args);

    core::Logger::instance().set_level(
        LogLevel(core::DefaultLogLevel + args.verbose_given));

    pipeline::ConverterConfig config;

    if (args.frame_size_given) {
        if (args.frame_size_arg <= 0) {
            roc_log(LogError, "invalid --frame-size: should be > 0");
            return 1;
        }
        config.internal_frame_size = (size_t)args.frame_size_arg;
    }

    sndio::SoxController::instance().set_buffer_size(config.internal_frame_size);

    core::HeapAllocator allocator;
    core::BufferPool<audio::sample_t> pool(allocator, config.internal_frame_size,
                                           args.poisoning_flag);

    sndio::SoxSource source(allocator, config.input_channels, 0,
                            config.internal_frame_size);
    if (!source.open(NULL, args.input_arg)) {
        roc_log(LogError, "can't open input file: %s", args.input_arg);
        return 1;
    }
    if (!source.is_file()) {
        roc_log(LogError, "not a file: %s", args.input_arg);
        return 1;
    }

    config.input_sample_rate = source.sample_rate();

    if (args.rate_given) {
        config.output_sample_rate = (size_t)args.rate_arg;
    } else {
        config.output_sample_rate = config.input_sample_rate;
    }

    switch ((unsigned)args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        config.resampler = audio::resampler_profile(audio::ResamplerProfile_Low);
        break;

    case resampler_profile_arg_medium:
        config.resampler = audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;

    case resampler_profile_arg_high:
        config.resampler = audio::resampler_profile(audio::ResamplerProfile_High);
        break;

    default:
        break;
    }

    if (args.resampler_interp_given) {
        config.resampler.window_interp = (size_t)args.resampler_interp_arg;
    }
    if (args.resampler_window_given) {
        config.resampler.window_size = (size_t)args.resampler_window_arg;
    }

    config.resampling = !args.no_resampling_flag;
    config.poisoning = args.poisoning_flag;

    audio::IWriter* output_writer = NULL;

    sndio::SoxSink sink(allocator, config.output_channels, config.output_sample_rate,
                        config.internal_frame_size);
    if (args.output_given) {
        if (!sink.open(NULL, args.output_arg)) {
            roc_log(LogError, "can't open output file: %s", args.output_arg);
            return 1;
        }
        if (!sink.is_file()) {
            roc_log(LogError, "not a file: %s", args.output_arg);
            return 1;
        }
        output_writer = &sink;
    }

    pipeline::Converter converter(config, output_writer, pool, allocator);
    if (!converter.valid()) {
        roc_log(LogError, "can't create converter pipeline");
        return 1;
    }

    sndio::Pump pump(pool, source, converter, source.frame_size(),
                     sndio::Pump::ModePermanent);
    if (!pump.valid()) {
        roc_log(LogError, "can't create audio pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
