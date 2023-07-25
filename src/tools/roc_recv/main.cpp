/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri.h"
#include "roc_address/io_uri.h"
#include "roc_audio/resampler_profile.h"
#include "roc_core/array.h"
#include "roc_core/crash_handler.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/parse_duration.h"
#include "roc_core/scoped_ptr.h"
#include "roc_netio/network_loop.h"
#include "roc_peer/context.h"
#include "roc_peer/receiver.h"
#include "roc_pipeline/converter_source.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/print_supported.h"
#include "roc_sndio/pump.h"

#include "roc_recv/cmdline.h"

using namespace roc;

int main(int argc, char** argv) {
    core::HeapAllocator::enable_panic_on_leak();

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

    peer::ContextConfig context_config;

    context_config.poisoning = args.poisoning_flag;

    if (args.packet_limit_given) {
        if (args.packet_limit_arg <= 0) {
            roc_log(LogError, "invalid --packet-limit: should be > 0");
            return 1;
        }
        context_config.max_packet_size = (size_t)args.packet_limit_arg;
    }

    if (args.frame_limit_given) {
        if (args.frame_limit_arg <= 0) {
            roc_log(LogError, "invalid --frame-limit: should be > 0");
            return 1;
        }
        context_config.max_frame_size = (size_t)args.frame_limit_arg;
    }

    core::HeapAllocator heap_allocator;

    peer::Context context(context_config, heap_allocator);
    if (!context.is_valid()) {
        roc_log(LogError, "can't initialize peer context");
        return 1;
    }

    sndio::BackendDispatcher backend_dispatcher;

    if (args.list_supported_given) {
        if (!sndio::print_supported(backend_dispatcher, context.allocator())) {
            return 1;
        }
        return 0;
    }

    pipeline::ReceiverConfig receiver_config;

    if (args.frame_length_given) {
        if (!core::parse_duration(args.frame_length_arg,
                                  receiver_config.common.internal_frame_length)) {
            roc_log(LogError, "invalid --frame-length: bad format");
            return 1;
        }
        if (receiver_config.common.output_sample_spec.ns_2_samples_overall(
                receiver_config.common.internal_frame_length)
            <= 0) {
            roc_log(LogError, "invalid --frame-length: should be > 0");
            return 1;
        }
    }

    sndio::BackendMap::instance().set_frame_size(
        receiver_config.common.internal_frame_length,
        receiver_config.common.output_sample_spec);

    if (args.sess_latency_given) {
        if (!core::parse_duration(args.sess_latency_arg,
                                  receiver_config.default_session.target_latency)) {
            roc_log(LogError, "invalid --sess-latency");
            return 1;
        }
    }

    if (args.min_latency_given) {
        if (!core::parse_duration(
                args.min_latency_arg,
                receiver_config.default_session.latency_monitor.min_latency)) {
            roc_log(LogError, "invalid --min-latency");
            return 1;
        }
    } else {
        receiver_config.default_session.latency_monitor.deduce_min_latency(
            receiver_config.default_session.target_latency);
    }

    if (args.max_latency_given) {
        if (!core::parse_duration(
                args.max_latency_arg,
                receiver_config.default_session.latency_monitor.max_latency)) {
            roc_log(LogError, "invalid --max-latency");
            return 1;
        }
    } else {
        receiver_config.default_session.latency_monitor.deduce_max_latency(
            receiver_config.default_session.target_latency);
    }

    if (args.np_timeout_given) {
        if (!core::parse_duration(
                args.np_timeout_arg,
                receiver_config.default_session.watchdog.no_playback_timeout)) {
            roc_log(LogError, "invalid --np-timeout");
            return 1;
        }
    }

    if (args.cp_timeout_given) {
        if (!core::parse_duration(
                args.cp_timeout_arg,
                receiver_config.default_session.watchdog.choppy_playback_timeout)) {
            roc_log(LogError, "invalid --cp-timeout");
            return 1;
        }
    }

    if (args.cp_window_given) {
        if (!core::parse_duration(
                args.cp_window_arg,
                receiver_config.default_session.watchdog.choppy_playback_window)) {
            roc_log(LogError, "invalid --cp-window");
            return 1;
        }
    } else {
        receiver_config.default_session.watchdog.deduce_choppy_playback_window(
            receiver_config.default_session.watchdog.choppy_playback_timeout);
    }

    receiver_config.common.enable_resampling = !args.no_resampling_flag;

    switch (args.resampler_backend_arg) {
    case resampler_backend_arg_default:
        receiver_config.default_session.resampler_backend =
            audio::ResamplerBackend_Default;
        break;
    case resampler_backend_arg_builtin:
        receiver_config.default_session.resampler_backend =
            audio::ResamplerBackend_Builtin;
        break;
    case resampler_backend_arg_speex:
        receiver_config.default_session.resampler_backend = audio::ResamplerBackend_Speex;
        break;
    default:
        break;
    }

    switch (args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        receiver_config.default_session.resampler_profile = audio::ResamplerProfile_Low;
        break;

    case resampler_profile_arg_medium:
        receiver_config.default_session.resampler_profile =
            audio::ResamplerProfile_Medium;
        break;

    case resampler_profile_arg_high:
        receiver_config.default_session.resampler_profile = audio::ResamplerProfile_High;
        break;

    default:
        break;
    }

    receiver_config.common.enable_poisoning = args.poisoning_flag;
    receiver_config.common.enable_profiling = args.profiling_flag;
    receiver_config.common.enable_beeping = args.beeping_flag;

    sndio::Config io_config;
    io_config.frame_length = receiver_config.common.internal_frame_length;
    io_config.sample_spec.set_channel_set(
        receiver_config.common.output_sample_spec.channel_set());

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
    } else {
        if (!receiver_config.common.enable_resampling) {
            io_config.sample_spec.set_sample_rate(pipeline::DefaultSampleRate);
        }
    }

    address::IoUri output_uri(context.allocator());
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
        output_sink.reset(backend_dispatcher.open_sink(output_uri, args.output_format_arg,
                                                       io_config, context.allocator()),
                          context.allocator());
    } else {
        output_sink.reset(
            backend_dispatcher.open_default_sink(io_config, context.allocator()),
            context.allocator());
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
    core::ScopedPtr<pipeline::ConverterSource> backup_pipeline;

    if (args.backup_given) {
        address::IoUri backup_uri(context.allocator());

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
            backend_dispatcher.open_source(backup_uri, args.backup_format_arg, io_config,
                                           context.allocator()),
            context.allocator());

        if (!backup_source) {
            roc_log(LogError, "can't open backup file or device: uri=%s format=%s",
                    args.backup_arg, args.backup_format_arg);
            return 1;
        }

        pipeline::ConverterConfig converter_config;

        converter_config.resampler_backend =
            receiver_config.default_session.resampler_backend;
        converter_config.resampler_profile =
            receiver_config.default_session.resampler_profile;

        converter_config.input_sample_spec =
            audio::SampleSpec(backup_source->sample_spec().sample_rate(),
                              receiver_config.common.output_sample_spec.channel_set());
        converter_config.output_sample_spec =
            audio::SampleSpec(receiver_config.common.output_sample_spec.sample_rate(),
                              receiver_config.common.output_sample_spec.channel_set());

        converter_config.internal_frame_length =
            receiver_config.common.internal_frame_length;

        converter_config.enable_resampling = receiver_config.common.enable_resampling;
        converter_config.enable_poisoning = receiver_config.common.enable_poisoning;

        backup_pipeline.reset(new (context.allocator()) pipeline::ConverterSource(
                                  converter_config, *backup_source,
                                  context.sample_buffer_factory(), context.allocator()),
                              context.allocator());
        if (!backup_pipeline) {
            roc_log(LogError, "can't create backup pipeline");
            return 1;
        }
    }

    peer::Receiver receiver(context, receiver_config);
    if (!receiver.is_valid()) {
        roc_log(LogError, "can't create receiver peer");
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
        address::EndpointUri endpoint(context.allocator());

        if (!address::parse_endpoint_uri(args.source_arg[slot],
                                         address::EndpointUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --source endpoint: %s", args.source_arg[slot]);
            return 1;
        }

        if (args.miface_given) {
            if (!receiver.set_multicast_group(slot, address::Iface_AudioSource,
                                              args.miface_arg[slot])) {
                roc_log(LogError, "can't set multicast group for --source endpoint: %s",
                        args.miface_arg[slot]);
                return 1;
            }
        }

        if (args.reuseaddr_given) {
            if (!receiver.set_reuseaddr(slot, address::Iface_AudioSource, true)) {
                roc_log(LogError, "can't set reuseaddr option for --source endpoint");
                return 1;
            }
        }

        if (!receiver.bind(slot, address::Iface_AudioSource, endpoint)) {
            roc_log(LogError, "can't bind --source endpoint: %s", args.source_arg[slot]);
            return 1;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.repair_given; slot++) {
        address::EndpointUri endpoint(context.allocator());

        if (!address::parse_endpoint_uri(args.repair_arg[slot],
                                         address::EndpointUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --repair endpoint: %s", args.source_arg[slot]);
            return 1;
        }

        if (args.miface_given) {
            if (!receiver.set_multicast_group(slot, address::Iface_AudioRepair,
                                              args.miface_arg[slot])) {
                roc_log(LogError, "can't set multicast group for --repair endpoint: %s",
                        args.miface_arg[slot]);
                return 1;
            }
        }

        if (args.reuseaddr_given) {
            if (!receiver.set_reuseaddr(slot, address::Iface_AudioRepair, true)) {
                roc_log(LogError, "can't set reuseaddr option for --repair endpoint");
                return 1;
            }
        }

        if (!receiver.bind(slot, address::Iface_AudioRepair, endpoint)) {
            roc_log(LogError, "can't bind --repair port: %s", args.repair_arg[slot]);
            return 1;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.control_given; slot++) {
        address::EndpointUri endpoint(context.allocator());

        if (!address::parse_endpoint_uri(args.control_arg[slot],
                                         address::EndpointUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --control endpoint: %s",
                    args.control_arg[slot]);
            return 1;
        }

        if (args.miface_given) {
            if (!receiver.set_multicast_group(slot, address::Iface_AudioControl,
                                              args.miface_arg[slot])) {
                roc_log(LogError, "can't set multicast group for --control endpoint: %s",
                        args.miface_arg[slot]);
                return 1;
            }
        }

        if (args.reuseaddr_given) {
            if (!receiver.set_reuseaddr(slot, address::Iface_AudioControl, true)) {
                roc_log(LogError, "can't set reuseaddr option for --control endpoint");
                return 1;
            }
        }

        if (!receiver.bind(slot, address::Iface_AudioControl, endpoint)) {
            roc_log(LogError, "can't bind --control endpoint: %s",
                    args.control_arg[slot]);
            return 1;
        }
    }

    sndio::Pump pump(
        context.sample_buffer_factory(), receiver.source(), backup_pipeline.get(),
        *output_sink, receiver_config.common.internal_frame_length,
        receiver_config.common.output_sample_spec,
        args.oneshot_flag ? sndio::Pump::ModeOneshot : sndio::Pump::ModePermanent);
    if (!pump.is_valid()) {
        roc_log(LogError, "can't create pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
