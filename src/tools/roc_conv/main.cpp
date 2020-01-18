/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_audio/resampler_profile.h"
#include "roc_core/colors.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/scoped_destructor.h"
#include "roc_core/scoped_ptr.h"
#include "roc_pipeline/converter_sink.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/print_supported.h"
#include "roc_sndio/pump.h"

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

    switch ((unsigned)args.color_arg) {
    case color_arg_auto:
        core::Logger::instance().set_colors(
            core::colors_available() ? core::ColorsEnabled : core::ColorsDisabled);
        break;

    case color_arg_always:
        core::Logger::instance().set_colors(core::ColorsMode(core::ColorsEnabled));
        break;

    case color_arg_never:
        core::Logger::instance().set_colors(core::ColorsMode(core::ColorsDisabled));
        break;

    default:
        break;
    }

    core::HeapAllocator allocator;

    if (args.list_supported_given) {
        if (!sndio::print_supported(allocator)) {
            return 1;
        }
        return 0;
    }

    pipeline::ConverterConfig config;

    if (args.frame_size_given) {
        if (args.frame_size_arg <= 0) {
            roc_log(LogError, "invalid --frame-size: should be > 0");
            return 1;
        }
        config.internal_frame_size = (size_t)args.frame_size_arg;
    }

    sndio::BackendDispatcher::instance().set_frame_size(config.internal_frame_size);

    core::BufferPool<audio::sample_t> pool(allocator, config.internal_frame_size,
                                           args.poisoning_flag);

    sndio::Config source_config;
    source_config.channels = config.input_channels;
    source_config.sample_rate = 0;
    source_config.frame_size = config.internal_frame_size;

    address::IoURI input(allocator);
    if (args.input_given) {
        if (!address::parse_io_uri(args.input_arg, input) || !input.is_file()) {
            roc_log(LogError, "invalid --input file URI");
            return 1;
        }
    }

    if (!args.input_format_given && input.is_special_file()) {
        roc_log(LogError, "--input-format should be specified if --input is \"-\"");
        return 1;
    }

    core::ScopedPtr<sndio::ISource> source(
        sndio::BackendDispatcher::instance().open_source(
            allocator, input, args.input_format_arg, source_config),
        allocator);
    if (!source) {
        roc_log(LogError, "can't open input: %s", args.input_arg);
        return 1;
    }
    if (source->has_clock()) {
        roc_log(LogError, "unsupported input: %s", args.input_arg);
        return 1;
    }

    config.input_sample_rate = source->sample_rate();

    if (args.rate_given) {
        config.output_sample_rate = (size_t)args.rate_arg;
    } else {
        config.output_sample_rate = config.input_sample_rate;
    }

    switch ((unsigned)args.resampler_backend_arg) {
    case resampler_backend_arg_builtin:
        config.resampler_backend = audio::ResamplerBackend_Builtin;
        break;

    default:
        break;
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

    sndio::Config sink_config;
    sink_config.channels = config.output_channels;
    sink_config.sample_rate = config.output_sample_rate;
    sink_config.frame_size = config.internal_frame_size;

    address::IoURI output(allocator);
    if (args.output_given) {
        if (!address::parse_io_uri(args.output_arg, output) || !output.is_file()) {
            roc_log(LogError, "invalid --output file URI");
            return 1;
        }
    }

    if (!args.output_format_given && output.is_special_file()) {
        roc_log(LogError, "--output-format should be specified if --output is \"-\"");
        return 1;
    }

    core::ScopedPtr<sndio::ISink> sink;
    if (args.output_given) {
        sink.reset(sndio::BackendDispatcher::instance().open_sink(
                       allocator, output, args.output_format_arg, sink_config),
                   allocator);
        if (!sink) {
            roc_log(LogError, "can't open output: %s", args.output_arg);
            return 1;
        }
        if (sink->has_clock()) {
            roc_log(LogError, "unsupported output: %s", args.output_arg);
            return 1;
        }
        output_writer = sink.get();
    }

    pipeline::ConverterSink converter(config, output_writer, pool, allocator);
    if (!converter.valid()) {
        roc_log(LogError, "can't create converter pipeline");
        return 1;
    }

    sndio::Pump pump(pool, *source, NULL, converter, config.internal_frame_size,
                     sndio::Pump::ModePermanent);
    if (!pump.valid()) {
        roc_log(LogError, "can't create audio pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
