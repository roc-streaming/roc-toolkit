/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri.h"
#include "roc_address/io_uri.h"
#include "roc_audio/resampler_profile.h"
#include "roc_core/array.h"
#include "roc_core/colors.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/parse_duration.h"
#include "roc_core/scoped_destructor.h"
#include "roc_core/scoped_ptr.h"
#include "roc_netio/network_loop.h"
#include "roc_peer/context.h"
#include "roc_peer/receiver.h"
#include "roc_pipeline/converter_source.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_sndio/backend_dispatcher.h"
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
    if (!context.valid()) {
        roc_log(LogError, "can't initialize peer context");
        return 1;
    }

    if (args.list_supported_given) {
        if (!sndio::print_supported(context.allocator())) {
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
        if (packet::ns_to_size(receiver_config.common.internal_frame_length,
                               receiver_config.common.output_sample_rate,
                               receiver_config.common.output_channels)
            <= 0) {
            roc_log(LogError, "invalid --frame-length: should be > 0");
            return 1;
        }
    }

    sndio::BackendDispatcher::instance().set_frame_size(
        receiver_config.common.internal_frame_length,
        receiver_config.common.output_sample_rate,
        receiver_config.common.output_channels);

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
        receiver_config.default_session.latency_monitor.min_latency =
            receiver_config.default_session.target_latency
            * pipeline::DefaultMinLatencyFactor;
    }

    if (args.max_latency_given) {
        if (!core::parse_duration(
                args.max_latency_arg,
                receiver_config.default_session.latency_monitor.max_latency)) {
            roc_log(LogError, "invalid --max-latency");
            return 1;
        }
    } else {
        receiver_config.default_session.latency_monitor.max_latency =
            receiver_config.default_session.target_latency
            * pipeline::DefaultMaxLatencyFactor;
    }

    if (args.np_timeout_given) {
        if (!core::parse_duration(
                args.np_timeout_arg,
                receiver_config.default_session.watchdog.no_playback_timeout)) {
            roc_log(LogError, "invalid --np-timeout");
            return 1;
        }
    }

    if (args.bp_timeout_given) {
        if (!core::parse_duration(
                args.bp_timeout_arg,
                receiver_config.default_session.watchdog.broken_playback_timeout)) {
            roc_log(LogError, "invalid --bp-timeout");
            return 1;
        }
    }

    if (args.bp_window_given) {
        if (!core::parse_duration(
                args.bp_window_arg,
                receiver_config.default_session.watchdog.breakage_detection_window)) {
            roc_log(LogError, "invalid --bp-window");
            return 1;
        }
    }

    receiver_config.common.resampling = !args.no_resampling_flag;

    switch ((unsigned)args.resampler_backend_arg) {
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

    switch ((unsigned)args.resampler_profile_arg) {
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

    receiver_config.common.poisoning = args.poisoning_flag;
    receiver_config.common.profiling = args.profiling_flag;
    receiver_config.common.beeping = args.beeping_flag;

    sndio::Config io_config;
    io_config.frame_length = receiver_config.common.internal_frame_length;
    io_config.sample_rate = receiver_config.common.output_sample_rate;
    io_config.channels = receiver_config.common.output_channels;

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
        io_config.sample_rate = (size_t)args.rate_arg;
    } else {
        if (!receiver_config.common.resampling) {
            io_config.sample_rate = pipeline::DefaultSampleRate;
        }
    }

    address::IoURI output_uri(context.allocator());
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

    core::ScopedPtr<sndio::ISink> output_sink(
        sndio::BackendDispatcher::instance().open_sink(context.allocator(), output_uri,
                                                       args.output_format_arg, io_config),
        context.allocator());
    if (!output_sink) {
        roc_log(LogError, "can't open output file or device: uri=%s format=%s",
                args.output_arg, args.output_format_arg);
        return 1;
    }

    receiver_config.common.timing = !output_sink->has_clock();
    receiver_config.common.output_sample_rate = output_sink->sample_rate();

    if (receiver_config.common.output_sample_rate == 0) {
        roc_log(LogError,
                "can't detect output sample rate, try to set it "
                "explicitly with --rate option");
        return 1;
    }

    core::ScopedPtr<sndio::ISource> backup_source;
    core::ScopedPtr<pipeline::ConverterSource> backup_pipeline;

    if (args.backup_given) {
        address::IoURI backup_uri(context.allocator());

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
            sndio::BackendDispatcher::instance().open_source(
                context.allocator(), backup_uri, args.backup_format_arg, io_config),
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

        converter_config.input_sample_rate = backup_source->sample_rate();
        converter_config.output_sample_rate = receiver_config.common.output_sample_rate;

        converter_config.input_channels = receiver_config.common.output_channels;
        converter_config.output_channels = receiver_config.common.output_channels;

        converter_config.internal_frame_length =
            receiver_config.common.internal_frame_length;

        converter_config.resampling = receiver_config.common.resampling;
        converter_config.poisoning = receiver_config.common.poisoning;

        backup_pipeline.reset(new (context.allocator()) pipeline::ConverterSource(
                                  converter_config, *backup_source,
                                  context.sample_buffer_pool(), context.allocator()),
                              context.allocator());
        if (!backup_pipeline) {
            roc_log(LogError, "can't create backup pipeline");
            return 1;
        }
    }

    peer::Receiver receiver(context, receiver_config);
    if (!receiver.valid()) {
        roc_log(LogError, "can't create receiver peer");
        return 1;
    }

    if (!args.source_given) {
        roc_log(LogError, "source port must be specified");
        return 1;
    }

    if (args.source_given) {
        address::EndpointURI endpoint(context.allocator());

        if (!address::parse_endpoint_uri(args.source_arg,
                                         address::EndpointURI::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse source endpoint: %s", args.source_arg);
            return 1;
        }

        if (args.miface_given) {
            if (!receiver.set_multicast_group(address::Iface_AudioSource,
                                              args.miface_arg)) {
                roc_log(LogError, "can't set miface: %s", args.miface_arg);
                return 1;
            }
        }
        if (!receiver.bind(address::Iface_AudioSource, endpoint)) {
            roc_log(LogError, "can't bind source endpoint: %s", args.source_arg);
            return 1;
        }
    }

    if (args.repair_given) {
        address::EndpointURI endpoint(context.allocator());

        if (!address::parse_endpoint_uri(args.repair_arg,
                                         address::EndpointURI::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse repair endpoint: %s", args.source_arg);
            return 1;
        }

        if (args.miface_given) {
            if (!receiver.set_multicast_group(address::Iface_AudioRepair,
                                              args.miface_arg)) {
                roc_log(LogError, "can't set miface: %s", args.miface_arg);
                return 1;
            }
        }
        if (!receiver.bind(address::Iface_AudioRepair, endpoint)) {
            roc_log(LogError, "can't bind repair port: %s", args.repair_arg);
            return 1;
        }
    }

    sndio::Pump pump(
        context.sample_buffer_pool(), receiver.source(), backup_pipeline.get(),
        *output_sink, receiver_config.common.internal_frame_length,
        receiver_config.common.output_sample_rate, receiver_config.common.output_channels,
        args.oneshot_flag ? sndio::Pump::ModeOneshot : sndio::Pump::ModePermanent);
    if (!pump.valid()) {
        roc_log(LogError, "can't create pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
