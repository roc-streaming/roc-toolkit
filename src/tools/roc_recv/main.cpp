/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_address/parse_socket_addr.h"
#include "roc_audio/resampler_profile.h"
#include "roc_core/array.h"
#include "roc_core/colors.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/parse_duration.h"
#include "roc_core/scoped_destructor.h"
#include "roc_core/scoped_ptr.h"
#include "roc_netio/transceiver.h"
#include "roc_pipeline/converter_source.h"
#include "roc_pipeline/parse_port.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/print_supported.h"
#include "roc_sndio/pump.h"

#include "roc_recv/cmdline.h"

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

    pipeline::ReceiverConfig config;

    size_t max_packet_size = 2048;
    if (args.packet_limit_given) {
        if (args.packet_limit_arg <= 0) {
            roc_log(LogError, "invalid --packet-limit: should be > 0");
            return 1;
        }
        max_packet_size = (size_t)args.packet_limit_arg;
    }

    if (args.frame_size_given) {
        if (args.frame_size_arg <= 0) {
            roc_log(LogError, "invalid --frame-size: should be > 0");
            return 1;
        }
        config.common.internal_frame_size = (size_t)args.frame_size_arg;
    }

    sndio::BackendDispatcher::instance().set_frame_size(
        config.common.internal_frame_size);

    if (args.sess_latency_given) {
        if (!core::parse_duration(args.sess_latency_arg,
                                  config.default_session.target_latency)) {
            roc_log(LogError, "invalid --sess-latency");
            return 1;
        }
    }

    if (args.min_latency_given) {
        if (!core::parse_duration(args.min_latency_arg,
                                  config.default_session.latency_monitor.min_latency)) {
            roc_log(LogError, "invalid --min-latency");
            return 1;
        }
    } else {
        config.default_session.latency_monitor.min_latency =
            config.default_session.target_latency * pipeline::DefaultMinLatencyFactor;
    }

    if (args.max_latency_given) {
        if (!core::parse_duration(args.max_latency_arg,
                                  config.default_session.latency_monitor.max_latency)) {
            roc_log(LogError, "invalid --max-latency");
            return 1;
        }
    } else {
        config.default_session.latency_monitor.max_latency =
            config.default_session.target_latency * pipeline::DefaultMaxLatencyFactor;
    }

    if (args.np_timeout_given) {
        if (!core::parse_duration(args.np_timeout_arg,
                                  config.default_session.watchdog.no_playback_timeout)) {
            roc_log(LogError, "invalid --np-timeout");
            return 1;
        }
    }

    if (args.bp_timeout_given) {
        if (!core::parse_duration(
                args.bp_timeout_arg,
                config.default_session.watchdog.broken_playback_timeout)) {
            roc_log(LogError, "invalid --bp-timeout");
            return 1;
        }
    }

    if (args.bp_window_given) {
        if (!core::parse_duration(
                args.bp_window_arg,
                config.default_session.watchdog.breakage_detection_window)) {
            roc_log(LogError, "invalid --bp-window");
            return 1;
        }
    }

    config.common.resampling = !args.no_resampling_flag;

    switch ((unsigned)args.resampler_backend_arg) {
    case resampler_backend_arg_builtin:
        config.default_session.resampler_backend = audio::ResamplerBackend_Builtin;
        break;

    default:
        break;
    }

    switch ((unsigned)args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        config.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_Low);
        break;

    case resampler_profile_arg_medium:
        config.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;

    case resampler_profile_arg_high:
        config.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_High);
        break;

    default:
        break;
    }

    if (args.resampler_interp_given) {
        if (args.resampler_interp_arg <= 0) {
            roc_log(LogError, "invalid --resampler-interp: should be > 0");
            return 1;
        }
        config.default_session.resampler.window_interp =
            (size_t)args.resampler_interp_arg;
    }

    if (args.resampler_window_given) {
        if (args.resampler_window_arg <= 0) {
            roc_log(LogError, "invalid --resampler-window: should be > 0");
            return 1;
        }
        config.default_session.resampler.window_size = (size_t)args.resampler_window_arg;
    }

    sndio::Config io_config;

    io_config.channels = config.common.output_channels;
    io_config.frame_size = config.common.internal_frame_size;

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
        if (!config.common.resampling) {
            io_config.sample_rate = pipeline::DefaultSampleRate;
        }
    }

    config.common.poisoning = args.poisoning_flag;
    config.common.beeping = args.beeping_flag;

    core::BufferPool<uint8_t> byte_buffer_pool(allocator, max_packet_size,
                                               args.poisoning_flag);
    core::BufferPool<audio::sample_t> sample_buffer_pool(
        allocator, config.common.internal_frame_size, args.poisoning_flag);
    packet::PacketPool packet_pool(allocator, args.poisoning_flag);

    address::IoURI output(allocator);
    if (args.output_given) {
        if (!address::parse_io_uri(args.output_arg, output)) {
            roc_log(LogError, "invalid --output file or device URI");
            return 1;
        }
    }

    if (args.output_format_given) {
        if (output.is_valid() && !output.is_file()) {
            roc_log(LogError,
                    "--output-format can't be used if --output is not a file URI");
            return 1;
        }
    } else {
        if (output.is_special_file()) {
            roc_log(LogError, "--output-format should be specified if --output is \"-\"");
            return 1;
        }
    }

    core::ScopedPtr<sndio::ISink> sink(
        sndio::BackendDispatcher::instance().open_sink(allocator, output,
                                                       args.output_format_arg, io_config),
        allocator);
    if (!sink) {
        roc_log(LogError, "can't open output file or device: uri=%s format=%s",
                args.output_arg, args.output_format_arg);
        return 1;
    }

    config.common.timing = !sink->has_clock();
    config.common.output_sample_rate = sink->sample_rate();

    if (config.common.output_sample_rate == 0) {
        roc_log(LogError,
                "can't detect output sample rate, try to set it "
                "explicitly with --rate option");
        return 1;
    }

    core::ScopedPtr<sndio::ISource> backup_source;
    core::ScopedPtr<pipeline::ConverterSource> backup_pipeline;

    if (args.backup_given) {
        address::IoURI backup(allocator);

        if (!address::parse_io_uri(args.backup_arg, backup)) {
            roc_log(LogError, "invalid --backup file or device URI");
            return 1;
        }

        if (args.backup_format_given) {
            if (backup.is_valid() && !backup.is_file()) {
                roc_log(LogError,
                        "--backup-format can't be used if --backup is not a file URI");
                return 1;
            }
        } else {
            if (backup.is_special_file()) {
                roc_log(LogError,
                        "--backup-format should be specified if --backup is \"-\"");
                return 1;
            }
        }

        backup_source.reset(sndio::BackendDispatcher::instance().open_source(
                                allocator, backup, args.backup_format_arg, io_config),
                            allocator);
        if (!backup_source) {
            roc_log(LogError, "can't open backup file or device: uri=%s format=%s",
                    args.backup_arg, args.backup_format_arg);
            return 1;
        }

        pipeline::ConverterConfig converter_config;

        converter_config.resampler = config.default_session.resampler;
        converter_config.resampler_backend = config.default_session.resampler_backend;

        converter_config.input_sample_rate = backup_source->sample_rate();
        converter_config.output_sample_rate = config.common.output_sample_rate;

        converter_config.input_channels = config.common.output_channels;
        converter_config.output_channels = config.common.output_channels;

        converter_config.internal_frame_size = config.common.internal_frame_size;

        converter_config.resampling = config.common.resampling;
        converter_config.poisoning = config.common.poisoning;

        backup_pipeline.reset(
            new (allocator) pipeline::ConverterSource(converter_config, *backup_source,
                                                      sample_buffer_pool, allocator),
            allocator);
        if (!backup_pipeline) {
            roc_log(LogError, "can't create backup pipeline");
            return 1;
        }
    }

    fec::CodecMap codec_map;
    rtp::FormatMap format_map;

    pipeline::ReceiverSource receiver(config, codec_map, format_map, packet_pool,
                                      byte_buffer_pool, sample_buffer_pool, allocator);
    if (!receiver.valid()) {
        roc_log(LogError, "can't create receiver pipeline");
        return 1;
    }

    sndio::Pump pump(sample_buffer_pool, receiver, backup_pipeline.get(), *sink,
                     config.common.internal_frame_size,
                     args.oneshot_flag ? sndio::Pump::ModeOneshot
                                       : sndio::Pump::ModePermanent);
    if (!pump.valid()) {
        roc_log(LogError, "can't create pump");
        return 1;
    }

    netio::Transceiver trx(packet_pool, byte_buffer_pool, allocator);
    if (!trx.valid()) {
        roc_log(LogError, "can't create network transceiver");
        return 1;
    }

    if (!args.source_given) {
        roc_log(LogError, "source port must be specified");
        return 1;
    }

    if (args.source_given) {
        pipeline::PortConfig port;

        if (!pipeline::parse_port(pipeline::Port_AudioSource, args.source_arg, port)) {
            roc_log(LogError, "can't parse source port: %s", args.source_arg);
            return 1;
        }
        if (args.miface_given) {
            if (!address::set_miface_from_string(args.miface_arg, port.address)) {
                roc_log(LogError, "can't parse miface: %s", args.miface_arg);
                return 1;
            }
        }
        if (!trx.add_udp_receiver(port.address, receiver)) {
            roc_log(LogError, "can't bind source port: %s", args.source_arg);
            return 1;
        }
        if (!receiver.add_port(port)) {
            roc_log(LogError, "can't initialize source port: %s", args.source_arg);
            return 1;
        }
    }

    if (args.repair_given) {
        pipeline::PortConfig port;

        if (!pipeline::parse_port(pipeline::Port_AudioRepair, args.repair_arg, port)) {
            roc_log(LogError, "can't parse repair port: %s", args.repair_arg);
            return 1;
        }

        if (args.miface_given) {
            if (!address::set_miface_from_string(args.miface_arg, port.address)) {
                roc_log(LogError, "can't parse miface: %s", args.miface_arg);
                return 1;
            }
        }
        if (!trx.add_udp_receiver(port.address, receiver)) {
            roc_log(LogError, "can't bind repair port: %s", args.repair_arg);
            return 1;
        }
        if (!receiver.add_port(port)) {
            roc_log(LogError, "can't initialize repair port: %s", args.repair_arg);
            return 1;
        }
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
