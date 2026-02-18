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
#include "roc_dbgio/print_supported.h"
#include "roc_pipeline/transcoder_sink.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/io_pump.h"
#include "roc_status/code_to_str.h"

#include "roc_copy/cmdline.h"

using namespace roc;

namespace {

void init_logger(const gengetopt_args_info& args) {
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
}

bool build_input_config(const gengetopt_args_info& args, sndio::IoConfig& input_config) {
    if (args.io_frame_len_given) {
        if (!core::parse_duration(args.io_frame_len_arg, input_config.frame_length)) {
            roc_log(LogError, "invalid --frame-len: bad format");
            return false;
        }
        if (input_config.frame_length <= 0) {
            roc_log(LogError, "invalid --frame-len: should be > 0");
            return false;
        }
    }

    return true;
}

bool build_output_config(const gengetopt_args_info& args,
                         const sndio::IoConfig& input_config,
                         sndio::IoConfig& output_config) {
    output_config = input_config;

    if (args.output_encoding_given) {
        if (!audio::parse_sample_spec(args.output_encoding_arg,
                                      output_config.sample_spec)) {
            roc_log(LogError, "invalid --output-encoding");
            return 1;
        }
    }

    return true;
}

bool build_transcoder_config(const gengetopt_args_info& args,
                             pipeline::TranscoderConfig& transcoder_config,
                             sndio::ISource& input_source,
                             sndio::ISink* output_sink) {
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

    transcoder_config.enable_profiling = args.prof_flag;

    transcoder_config.input_sample_spec = input_source.sample_spec();
    transcoder_config.output_sample_spec =
        output_sink ? output_sink->sample_spec() : input_source.sample_spec();

    return true;
}

size_t compute_max_frame_size(const sndio::IoConfig& io_config) {
    audio::SampleSpec spec = io_config.sample_spec;
    spec.use_defaults(audio::Format_Pcm, audio::PcmSubformat_Raw,
                      audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                      audio::ChanMask_Surround_7_1_4, 48000);

    return spec.ns_2_samples_overall(io_config.frame_length) * sizeof(audio::sample_t);
}

bool parse_input_uri(const gengetopt_args_info& args, address::IoUri& input_uri) {
    if (!args.input_given) {
        roc_log(LogError, "missing mandatory --input URI");
        return false;
    }

    if (!address::parse_io_uri(args.input_arg, input_uri)) {
        roc_log(LogError, "invalid --input URI: bad format");
        return false;
    }

    if (!input_uri.is_file()) {
        roc_log(LogError, "invalid --input URI: should be file");
        return false;
    }

    return true;
}

bool parse_output_uri(const gengetopt_args_info& args, address::IoUri& output_uri) {
    if (!address::parse_io_uri(args.output_arg, output_uri)) {
        roc_log(LogError, "invalid --output URI: bad format");
        return false;
    }

    if (!output_uri.is_file()) {
        roc_log(LogError, "invalid --output URI: should be file");
        return false;
    }

    if (output_uri.is_special_file()) {
        if (!args.output_encoding_given) {
            roc_log(LogError, "--output-encoding is required when --output is \"-\"");
            return false;
        }
    }

    return true;
}

bool open_input_source(sndio::BackendDispatcher& backend_dispatcher,
                       const sndio::IoConfig& io_config,
                       const address::IoUri& input_uri,
                       core::ScopedPtr<sndio::ISource>& input_source) {
    const status::StatusCode code =
        backend_dispatcher.open_source(input_uri, io_config, input_source);

    if (code != status::StatusOK) {
        roc_log(LogError, "can't open --input file or device: status=%s",
                status::code_to_str(code));
        return false;
    }

    if (input_source->has_clock()) {
        roc_log(LogError, "unsupported --input type");
        return false;
    }

    return true;
}

bool open_output_sink(sndio::BackendDispatcher& backend_dispatcher,
                      const sndio::IoConfig& io_config,
                      const address::IoUri& output_uri,
                      core::ScopedPtr<sndio::ISink>& output_sink) {
    const status::StatusCode code =
        backend_dispatcher.open_sink(output_uri, io_config, output_sink);

    if (code != status::StatusOK) {
        roc_log(LogError, "can't open --output file or device: status=%s",
                status::code_to_str(code));
        return false;
    }

    if (output_sink->has_clock()) {
        roc_log(LogError, "unsupported --output type");
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char** argv) {
    core::CrashHandler crash_handler;

    core::HeapArena::set_guards(core::HeapArena_DefaultGuards
                                | core::HeapArena_LeakGuard);
    core::HeapArena heap_arena;

    gengetopt_args_info args;
    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }
    core::ScopedRelease<gengetopt_args_info> args_releaser(&args, &cmdline_parser_free);

    init_logger(args);

    sndio::IoConfig input_config;
    if (!build_input_config(args, input_config)) {
        return 1;
    }

    core::SlabPool<audio::Frame> frame_pool("frame_pool", heap_arena);
    core::SlabPool<core::Buffer> frame_buffer_pool(
        "frame_buffer_pool", heap_arena,
        sizeof(core::Buffer) + compute_max_frame_size(input_config));

    sndio::BackendDispatcher backend_dispatcher(frame_pool, frame_buffer_pool,
                                                heap_arena);

    if (args.list_supported_given) {
        if (!dbgio::print_supported(dbgio::Print_Sndio | dbgio::Print_Audio,
                                    backend_dispatcher, heap_arena)) {
            return 1;
        }
        return 0;
    }

    address::IoUri input_uri(heap_arena);
    if (!parse_input_uri(args, input_uri)) {
        return 1;
    }

    core::ScopedPtr<sndio::ISource> input_source;
    if (!open_input_source(backend_dispatcher, input_config, input_uri, input_source)) {
        return 1;
    }

    input_config.sample_spec = input_source->sample_spec();
    input_config.frame_length = input_source->frame_length();

    sndio::IoConfig output_config;
    if (!build_output_config(args, input_config, output_config)) {
        return 1;
    }

    address::IoUri output_uri(heap_arena);
    if (args.output_given) {
        if (!parse_output_uri(args, output_uri)) {
            return 1;
        }
    }

    core::ScopedPtr<sndio::ISink> output_sink;
    if (args.output_given) {
        if (!open_output_sink(backend_dispatcher, output_config, output_uri,
                              output_sink)) {
            return 1;
        }
        output_config.sample_spec = output_sink->sample_spec();
    }

    pipeline::TranscoderConfig transcoder_config;
    if (!build_transcoder_config(args, transcoder_config, *input_source,
                                 output_sink.get())) {
        return 1;
    }

    audio::ProcessorMap processor_map(heap_arena);

    pipeline::TranscoderSink transcoder(transcoder_config, output_sink.get(),
                                        processor_map, frame_pool, frame_buffer_pool,
                                        heap_arena);
    if (transcoder.init_status() != status::StatusOK) {
        roc_log(LogError, "can't create transcoder pipeline: status=%s",
                status::code_to_str(transcoder.init_status()));
        return 1;
    }

    sndio::IoPump pump(frame_pool, frame_buffer_pool, *input_source, NULL, transcoder,
                       input_config, sndio::IoPump::ModePermanent);
    if (pump.init_status() != status::StatusOK) {
        roc_log(LogError, "can't create io pump: status=%s",
                status::code_to_str(pump.init_status()));
        return 1;
    }

    const status::StatusCode status = pump.run();
    if (status != status::StatusOK) {
        roc_log(LogError, "io pump failed: status=%s", status::code_to_str(status));
        return 1;
    }

    return 0;
}
