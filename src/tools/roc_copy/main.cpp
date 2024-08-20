/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_core/crash_handler.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"
#include "roc_core/parse_units.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/scoped_release.h"
#include "roc_pipeline/transcoder_sink.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/io_config.h"
#include "roc_sndio/io_pump.h"
#include "roc_sndio/print_supported.h"
#include "roc_status/code_to_str.h"

#include "roc_copy/cmdline.h"

using namespace roc;

int main(int argc, char** argv) {
    core::HeapArena::set_guards(core::HeapArena_DefaultGuards
                                | core::HeapArena_LeakGuard);

    core::HeapArena arena;

    core::CrashHandler crash_handler;

    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::ScopedRelease<gengetopt_args_info> args_holder(&args, &cmdline_parser_free);

    core::Logger::instance().set_verbosity(args.verbose_given);

    switch (args.color_arg) {
    case color_arg_auto:
        core::Logger::instance().set_colors(core::ColorsAuto);
        break;
    case color_arg_always:
        core::Logger::instance().set_colors(core::ColorsEnabled);
        break;
    case color_arg_never:
        core::Logger::instance().set_colors(core::ColorsDisabled);
        break;
    default:
        break;
    }

    pipeline::TranscoderConfig transcoder_config;

    sndio::IoConfig source_config;

    if (args.frame_len_given) {
        if (!core::parse_duration(args.frame_len_arg, source_config.frame_length)) {
            roc_log(LogError, "invalid --frame-len: bad format");
            return 1;
        }
        if (source_config.frame_length <= 0) {
            roc_log(LogError, "invalid --frame-len: should be > 0");
            return 1;
        }
    }

    switch (args.resampler_backend_arg) {
    case resampler_backend_arg_default:
        transcoder_config.resampler.backend = audio::ResamplerBackend_Auto;
        break;
    case resampler_backend_arg_builtin:
        transcoder_config.resampler.backend = audio::ResamplerBackend_Builtin;
        break;
    case resampler_backend_arg_speex:
        transcoder_config.resampler.backend = audio::ResamplerBackend_Speex;
        break;
    case resampler_backend_arg_speexdec:
        transcoder_config.resampler.backend = audio::ResamplerBackend_SpeexDec;
        break;
    default:
        break;
    }

    switch (args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        transcoder_config.resampler.profile = audio::ResamplerProfile_Low;
        break;
    case resampler_profile_arg_medium:
        transcoder_config.resampler.profile = audio::ResamplerProfile_Medium;
        break;
    case resampler_profile_arg_high:
        transcoder_config.resampler.profile = audio::ResamplerProfile_High;
        break;
    default:
        break;
    }

    transcoder_config.enable_profiling = args.profile_flag;

    core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
    core::SlabPool<core::Buffer> frame_buffer_pool(
        "frame_buffer_pool", arena,
        sizeof(core::Buffer)
            + transcoder_config.input_sample_spec.ns_2_bytes(source_config.frame_length));

    sndio::BackendDispatcher backend_dispatcher(frame_pool, frame_buffer_pool, arena);

    sndio::BackendMap::instance().set_frame_size(source_config.frame_length,
                                                 transcoder_config.input_sample_spec);

    if (args.list_supported_given) {
        if (!sndio::print_supported(backend_dispatcher, arena)) {
            return 1;
        }
        return 0;
    }

    address::IoUri input_uri(arena);
    if (!address::parse_io_uri(args.input_arg, input_uri) || !input_uri.is_file()) {
        roc_log(LogError, "invalid --input file URI");
        return 1;
    }
    if (!args.input_format_given && input_uri.is_special_file()) {
        roc_log(LogError, "--input-format should be specified if --input is \"-\"");
        return 1;
    }

    core::ScopedPtr<sndio::ISource> input_source;
    {
        const status::StatusCode code = backend_dispatcher.open_source(
            input_uri, args.input_format_arg, source_config, input_source);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open --input file or device: status=%s",
                    status::code_to_str(code));
            return 1;
        }

        if (input_source->has_clock()) {
            roc_log(LogError, "unsupported --input type");
            return 1;
        }
    }

    transcoder_config.input_sample_spec = input_source->sample_spec();

    if (args.output_encoding_given) {
        if (!audio::parse_sample_spec(args.output_encoding_arg,
                                      transcoder_config.output_sample_spec)) {
            roc_log(LogError, "invalid --output-encoding");
            return 1;
        }
    } else {
        transcoder_config.output_sample_spec = transcoder_config.input_sample_spec;
    }

    audio::IFrameWriter* output_writer = NULL;

    sndio::IoConfig sink_config;
    sink_config.sample_spec = transcoder_config.output_sample_spec;
    sink_config.frame_length = source_config.frame_length;

    address::IoUri output_uri(arena);
    if (args.output_given) {
        if (!address::parse_io_uri(args.output_arg, output_uri)
            || !output_uri.is_file()) {
            roc_log(LogError, "invalid --output file URI");
            return 1;
        }
    }
    if (!args.output_format_given && output_uri.is_special_file()) {
        roc_log(LogError, "--output-format should be specified if --output is \"-\"");
        return 1;
    }

    core::ScopedPtr<sndio::ISink> output_sink;
    if (args.output_given) {
        const status::StatusCode code = backend_dispatcher.open_sink(
            output_uri, args.output_format_arg, sink_config, output_sink);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open --output file or device: status=%s",
                    status::code_to_str(code));
            return 1;
        }

        if (output_sink->has_clock()) {
            roc_log(LogError, "unsupported --output type");
            return 1;
        }

        output_writer = output_sink.get();
    }

    audio::ProcessorMap processor_map(arena);

    pipeline::TranscoderSink transcoder(transcoder_config, output_writer, processor_map,
                                        frame_pool, frame_buffer_pool, arena);
    if (transcoder.init_status() != status::StatusOK) {
        roc_log(LogError, "can't create transcoder pipeline: status=%s",
                status::code_to_str(transcoder.init_status()));
        return 1;
    }

    sndio::IoConfig pump_config;
    pump_config.sample_spec = input_source->sample_spec();
    pump_config.frame_length = source_config.frame_length;

    sndio::IoPump pump(frame_pool, frame_buffer_pool, *input_source, NULL, transcoder,
                       pump_config, sndio::IoPump::ModePermanent);
    if (pump.init_status() != status::StatusOK) {
        roc_log(LogError, "can't create audio pump: status=%s",
                status::code_to_str(pump.init_status()));
        return 1;
    }

    const status::StatusCode status = pump.run();
    if (status != status::StatusOK) {
        roc_log(LogError, "can't run audio pump: status=%s", status::code_to_str(status));
        return 1;
    }

    return 0;
}
