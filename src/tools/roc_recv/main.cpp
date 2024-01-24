/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri.h"
#include "roc_address/io_uri.h"
#include "roc_address/print_supported.h"
#include "roc_address/protocol_map.h"
#include "roc_core/crash_handler.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"
#include "roc_core/parse_units.h"
#include "roc_core/scoped_ptr.h"
#include "roc_netio/network_loop.h"
#include "roc_node/context.h"
#include "roc_node/receiver.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_pipeline/transcoder_source.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/print_supported.h"
#include "roc_sndio/pump.h"

#include "roc_recv/cmdline.h"

using namespace roc;

int main(int argc, char** argv) {
    core::HeapArena::set_flags(core::DefaultHeapArenaFlags
                               | core::HeapArenaFlag_EnableLeakDetection);

    core::HeapArena heap_arena;

    core::CrashHandler crash_handler;

    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::ScopedPtr<gengetopt_args_info, core::CustomAllocation> args_holder(
        &args, &cmdline_parser_free);

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

    pipeline::ReceiverConfig receiver_config;

    sndio::Config io_config;
    io_config.sample_spec.set_sample_format(
        receiver_config.common.output_sample_spec.sample_format());
    io_config.sample_spec.set_pcm_format(
        receiver_config.common.output_sample_spec.pcm_format());
    io_config.sample_spec.set_channel_set(
        receiver_config.common.output_sample_spec.channel_set());

    if (args.frame_len_given) {
        if (!core::parse_duration(args.frame_len_arg, io_config.frame_length)) {
            roc_log(LogError, "invalid --frame-len: bad format");
            return 1;
        }
        if (receiver_config.common.output_sample_spec.ns_2_samples_overall(
                io_config.frame_length)
            <= 0) {
            roc_log(LogError, "invalid --frame-len: should be > 0");
            return 1;
        }
    }

    if (args.io_latency_given) {
        if (!core::parse_duration(args.io_latency_arg, io_config.latency)) {
            roc_log(LogError, "invalid --io-latency");
            return 1;
        }
    }

    if (args.rate_given) {
        if (args.rate_arg <= 0) {
            roc_log(LogError, "invalid --rate: should be > 0");
            return 1;
        }
        io_config.sample_spec.set_sample_rate((size_t)args.rate_arg);
    }

    sndio::BackendMap::instance().set_frame_size(
        io_config.frame_length, receiver_config.common.output_sample_spec);

    if (args.sess_latency_given) {
        if (!core::parse_duration(
                args.sess_latency_arg,
                receiver_config.default_session.latency.target_latency)) {
            roc_log(LogError, "invalid --sess-latency");
            return 1;
        }
    }

    if (args.latency_tolerance_given) {
        if (!core::parse_duration(
                args.latency_tolerance_arg,
                receiver_config.default_session.latency.latency_tolerance)) {
            roc_log(LogError, "invalid --latency-tolerance");
            return 1;
        }
    }

    if (args.no_play_timeout_given) {
        if (!core::parse_duration(
                args.no_play_timeout_arg,
                receiver_config.default_session.watchdog.no_playback_timeout)) {
            roc_log(LogError, "invalid --no-play-timeout");
            return 1;
        }
    }

    if (args.choppy_play_timeout_given) {
        if (!core::parse_duration(
                args.choppy_play_timeout_arg,
                receiver_config.default_session.watchdog.choppy_playback_timeout)) {
            roc_log(LogError, "invalid --choppy-play-timeout");
            return 1;
        }
    }

    switch (args.clock_backend_arg) {
    case clock_backend_arg_disable:
        receiver_config.default_session.latency.fe_input =
            audio::FreqEstimatorInput_Disable;
        break;
    case clock_backend_arg_niq:
        receiver_config.default_session.latency.fe_input =
            audio::FreqEstimatorInput_NiqLatency;
        break;
    default:
        break;
    }

    switch (args.clock_profile_arg) {
    case clock_profile_arg_default:
        receiver_config.default_session.latency.fe_profile =
            audio::FreqEstimatorProfile_Default;
        break;
    case clock_profile_arg_responsive:
        receiver_config.default_session.latency.fe_profile =
            audio::FreqEstimatorProfile_Responsive;
        break;
    case clock_profile_arg_gradual:
        receiver_config.default_session.latency.fe_profile =
            audio::FreqEstimatorProfile_Gradual;
        break;
    default:
        break;
    }

    switch (args.resampler_backend_arg) {
    case resampler_backend_arg_default:
        receiver_config.default_session.resampler.backend =
            audio::ResamplerBackend_Default;
        break;
    case resampler_backend_arg_builtin:
        receiver_config.default_session.resampler.backend =
            audio::ResamplerBackend_Builtin;
        break;
    case resampler_backend_arg_speex:
        receiver_config.default_session.resampler.backend = audio::ResamplerBackend_Speex;
        break;
    case resampler_backend_arg_speexdec:
        receiver_config.default_session.resampler.backend =
            audio::ResamplerBackend_SpeexDec;
        break;
    default:
        break;
    }

    switch (args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        receiver_config.default_session.resampler.profile = audio::ResamplerProfile_Low;
        break;
    case resampler_profile_arg_medium:
        receiver_config.default_session.resampler.profile =
            audio::ResamplerProfile_Medium;
        break;
    case resampler_profile_arg_high:
        receiver_config.default_session.resampler.profile = audio::ResamplerProfile_High;
        break;

    default:
        break;
    }

    receiver_config.common.enable_profiling = args.profiling_flag;
    receiver_config.common.enable_beeping = args.beep_flag;

    node::ContextConfig context_config;

    if (args.max_packet_size_given) {
        if (!core::parse_size(args.max_packet_size_arg, context_config.max_packet_size)) {
            roc_log(LogError, "invalid --max-packet-size");
            return 1;
        }
        if (context_config.max_packet_size == 0) {
            roc_log(LogError, "invalid --max-packet-size: should be > 0");
            return 1;
        }
    }

    if (args.max_frame_size_given) {
        if (!core::parse_size(args.max_frame_size_arg, context_config.max_frame_size)) {
            roc_log(LogError, "invalid --max-frame-size");
            return 1;
        }
        if (context_config.max_frame_size == 0) {
            roc_log(LogError, "invalid --max-frame-size: should be > 0");
            return 1;
        }
    } else {
        audio::SampleSpec spec = io_config.sample_spec;
        if (spec.sample_rate() == 0) {
            spec.set_sample_rate(48000);
        }
        if (spec.num_channels() == 0) {
            spec.set_channel_set(audio::ChannelSet(audio::ChanLayout_Surround,
                                                   audio::ChanOrder_Smpte,
                                                   audio::ChanMask_Surround_7_1_4));
        }
        context_config.max_frame_size =
            spec.ns_2_samples_overall(io_config.frame_length) * sizeof(audio::sample_t);
    }

    node::Context context(context_config, heap_arena);
    if (!context.is_valid()) {
        roc_log(LogError, "can't initialize node context");
        return 1;
    }

    sndio::BackendDispatcher backend_dispatcher(context.arena());

    if (args.list_supported_given) {
        if (!address::print_supported(context.arena())) {
            return 1;
        }

        if (!sndio::print_supported(backend_dispatcher, context.arena())) {
            return 1;
        }

        return 0;
    }

    address::IoUri output_uri(context.arena());
    if (args.output_given) {
        if (!address::parse_io_uri(args.output_arg, output_uri)) {
            roc_log(LogError, "invalid --output file or device URI");
            return 1;
        }
    }

    if (args.output_format_given) {
        if (output_uri.is_valid() && !output_uri.is_file()) {
            roc_log(LogError,
                    "--output-format can't be used if --output is not a file URI");
            return 1;
        }
    } else {
        if (output_uri.is_special_file()) {
            roc_log(LogError, "--output-format should be specified if --output is \"-\"");
            return 1;
        }
    }

    core::ScopedPtr<sndio::ISink> output_sink;
    if (output_uri.is_valid()) {
        output_sink.reset(
            backend_dispatcher.open_sink(output_uri, args.output_format_arg, io_config),
            context.arena());
    } else {
        output_sink.reset(backend_dispatcher.open_default_sink(io_config),
                          context.arena());
    }
    if (!output_sink) {
        roc_log(LogError, "can't open output file or device: uri=%s format=%s",
                args.output_arg, args.output_format_arg);
        return 1;
    }

    receiver_config.common.enable_timing = !output_sink->has_clock();
    receiver_config.common.output_sample_spec.set_sample_rate(
        output_sink->sample_spec().sample_rate());

    if (receiver_config.common.output_sample_spec.sample_rate() == 0) {
        roc_log(LogError,
                "can't detect output sample rate, try to set it "
                "explicitly with --rate option");
        return 1;
    }

    core::ScopedPtr<sndio::ISource> backup_source;
    core::ScopedPtr<pipeline::TranscoderSource> backup_pipeline;

    if (args.backup_given) {
        address::IoUri backup_uri(context.arena());

        if (!address::parse_io_uri(args.backup_arg, backup_uri)) {
            roc_log(LogError, "invalid --backup file or device URI");
            return 1;
        }

        if (args.backup_format_given) {
            if (backup_uri.is_valid() && !backup_uri.is_file()) {
                roc_log(LogError,
                        "--backup-format can't be used if --backup is not a file URI");
                return 1;
            }
        } else {
            if (backup_uri.is_special_file()) {
                roc_log(LogError,
                        "--backup-format should be specified if --backup is \"-\"");
                return 1;
            }
        }

        backup_source.reset(
            backend_dispatcher.open_source(backup_uri, args.backup_format_arg, io_config),
            context.arena());

        if (!backup_source) {
            roc_log(LogError, "can't open backup file or device: uri=%s format=%s",
                    args.backup_arg, args.backup_format_arg);
            return 1;
        }

        pipeline::TranscoderConfig transcoder_config;

        transcoder_config.resampler.backend =
            receiver_config.default_session.resampler.backend;
        transcoder_config.resampler.profile =
            receiver_config.default_session.resampler.profile;

        transcoder_config.input_sample_spec =
            audio::SampleSpec(backup_source->sample_spec().sample_rate(),
                              receiver_config.common.output_sample_spec.pcm_format(),
                              receiver_config.common.output_sample_spec.channel_set());
        transcoder_config.output_sample_spec =
            audio::SampleSpec(receiver_config.common.output_sample_spec.sample_rate(),
                              receiver_config.common.output_sample_spec.pcm_format(),
                              receiver_config.common.output_sample_spec.channel_set());

        backup_pipeline.reset(new (context.arena()) pipeline::TranscoderSource(
                                  transcoder_config, *backup_source,
                                  context.sample_buffer_factory(), context.arena()),
                              context.arena());
        if (!backup_pipeline) {
            roc_log(LogError, "can't create backup pipeline");
            return 1;
        }
    }

    node::Receiver receiver(context, receiver_config);
    if (!receiver.is_valid()) {
        roc_log(LogError, "can't create receiver node");
        return 1;
    }

    if (args.source_given == 0) {
        roc_log(LogError, "at least one --source endpoint should be specified");
        return 1;
    }

    if (args.repair_given != 0 && args.repair_given != args.source_given) {
        roc_log(LogError,
                "invalid number of --repair endpoints: expected either 0 or %d endpoints"
                " (one per --source), got %d endpoints",
                (int)args.source_given, (int)args.repair_given);
        return 1;
    }

    if (args.control_given != 0 && args.control_given != args.source_given) {
        roc_log(LogError,
                "invalid number of --control endpoints: expected either 0 or %d endpoints"
                " (one per --source), got %d endpoints",
                (int)args.source_given, (int)args.control_given);
        return 1;
    }

    if (args.miface_given != 0 && args.miface_given != args.source_given) {
        roc_log(LogError,
                "invalid number of --miface values: expected either 0 or %d values"
                " (one per --source), got %d values",
                (int)args.source_given, (int)args.miface_given);
        return 1;
    }

    for (size_t slot = 0; slot < (size_t)args.source_given; slot++) {
        address::EndpointUri endpoint(context.arena());

        if (!address::parse_endpoint_uri(args.source_arg[slot],
                                         address::EndpointUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --source endpoint: %s", args.source_arg[slot]);
            return 1;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (args.miface_given) {
            if (strlen(args.miface_arg[slot])
                >= sizeof(iface_config.multicast_interface)) {
                roc_log(LogError, "invalid --miface \"%s\": string too long",
                        args.miface_arg[slot]);
                return 1;
            }
            strcpy(iface_config.multicast_interface, args.miface_arg[slot]);
        }

        if (!receiver.configure(slot, address::Iface_AudioSource, iface_config)) {
            roc_log(LogError, "can't configure --source endpoint");
            return 1;
        }

        if (!receiver.bind(slot, address::Iface_AudioSource, endpoint)) {
            roc_log(LogError, "can't bind --source endpoint: %s", args.source_arg[slot]);
            return 1;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.repair_given; slot++) {
        address::EndpointUri endpoint(context.arena());

        if (!address::parse_endpoint_uri(args.repair_arg[slot],
                                         address::EndpointUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --repair endpoint: %s", args.source_arg[slot]);
            return 1;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (args.miface_given) {
            if (strlen(args.miface_arg[slot])
                >= sizeof(iface_config.multicast_interface)) {
                roc_log(LogError, "invalid --miface \"%s\": string too long",
                        args.miface_arg[slot]);
                return 1;
            }
            strcpy(iface_config.multicast_interface, args.miface_arg[slot]);
        }

        if (!receiver.configure(slot, address::Iface_AudioRepair, iface_config)) {
            roc_log(LogError, "can't configure --repair endpoint");
            return 1;
        }

        if (!receiver.bind(slot, address::Iface_AudioRepair, endpoint)) {
            roc_log(LogError, "can't bind --repair port: %s", args.repair_arg[slot]);
            return 1;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.control_given; slot++) {
        address::EndpointUri endpoint(context.arena());

        if (!address::parse_endpoint_uri(args.control_arg[slot],
                                         address::EndpointUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --control endpoint: %s",
                    args.control_arg[slot]);
            return 1;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (args.miface_given) {
            if (strlen(args.miface_arg[slot])
                >= sizeof(iface_config.multicast_interface)) {
                roc_log(LogError, "invalid --miface \"%s\": string too long",
                        args.miface_arg[slot]);
                return 1;
            }
            strcpy(iface_config.multicast_interface, args.miface_arg[slot]);
        }

        if (!receiver.configure(slot, address::Iface_AudioControl, iface_config)) {
            roc_log(LogError, "can't configure --control endpoint");
            return 1;
        }

        if (!receiver.bind(slot, address::Iface_AudioControl, endpoint)) {
            roc_log(LogError, "can't bind --control endpoint: %s",
                    args.control_arg[slot]);
            return 1;
        }
    }

    sndio::Pump pump(
        context.sample_buffer_factory(), receiver.source(), backup_pipeline.get(),
        *output_sink, io_config.frame_length, receiver_config.common.output_sample_spec,
        args.oneshot_flag ? sndio::Pump::ModeOneshot : sndio::Pump::ModePermanent);
    if (!pump.is_valid()) {
        roc_log(LogError, "can't create pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
