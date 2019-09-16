/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_profile.h"
#include "roc_core/array.h"
#include "roc_core/colors.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/parse_duration.h"
#include "roc_core/scoped_destructor.h"
#include "roc_core/unique_ptr.h"
#include "roc_netio/transceiver.h"
#include "roc_pipeline/parse_port.h"
#include "roc_pipeline/port_utils.h"
#include "roc_pipeline/sender.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/print_drivers.h"
#include "roc_sndio/pump.h"

#include "roc_send/cmdline.h"

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

    if (args.list_drivers_given) {
        if (!sndio::print_drivers(allocator)) {
            return 1;
        }
        return 0;
    }

    pipeline::SenderConfig config;

    if (args.packet_length_given) {
        if (!core::parse_duration(args.packet_length_arg, config.packet_length)) {
            roc_log(LogError, "invalid --packet-length");
            return 1;
        }
    }

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
        config.internal_frame_size = (size_t)args.frame_size_arg;
    }

    sndio::BackendDispatcher::instance().set_frame_size(config.internal_frame_size);

    pipeline::PortConfig source_port;
    if (args.source_given) {
        if (!pipeline::parse_port(pipeline::Port_AudioSource, args.source_arg,
                                  source_port)) {
            roc_log(LogError, "can't parse remote source port: %s", args.source_arg);
            return 1;
        }
    }

    pipeline::PortConfig repair_port;
    if (args.repair_given) {
        if (!pipeline::parse_port(pipeline::Port_AudioRepair, args.repair_arg,
                                  repair_port)) {
            roc_log(LogError, "can't parse remote repair port: %s", args.repair_arg);
            return 1;
        }
    }

    config.fec_encoder.scheme = pipeline::port_fec_scheme(source_port.protocol);

    if (args.nbsrc_given) {
        if (config.fec_encoder.scheme == packet::FEC_None) {
            roc_log(LogError, "--nbsrc can't be used when fec is disabled)");
            return 1;
        }
        if (args.nbsrc_arg <= 0) {
            roc_log(LogError, "invalid --nbsrc: should be > 0");
            return 1;
        }
        config.fec_writer.n_source_packets = (size_t)args.nbsrc_arg;
    }

    if (args.nbrpr_given) {
        if (config.fec_encoder.scheme == packet::FEC_None) {
            roc_log(LogError, "--nbrpr can't be used when fec is disabled");
            return 1;
        }
        if (args.nbrpr_arg <= 0) {
            roc_log(LogError, "invalid --nbrpr: should be > 0");
            return 1;
        }
        config.fec_writer.n_repair_packets = (size_t)args.nbrpr_arg;
    }

    config.resampling = !args.no_resampling_flag;

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
        roc_panic("unexpected resampler profile");
    }

    if (args.resampler_interp_given) {
        if (args.resampler_interp_arg <= 0) {
            roc_log(LogError, "invalid --resampler-interp: should be > 0");
            return 1;
        }
        config.resampler.window_interp = (size_t)args.resampler_interp_arg;
    }

    if (args.resampler_window_given) {
        if (args.resampler_window_arg <= 0) {
            roc_log(LogError, "invalid --resampler-window: should be > 0");
            return 1;
        }
        config.resampler.window_size = (size_t)args.resampler_window_arg;
    }

    sndio::Config source_config;
    source_config.channels = config.input_channels;
    source_config.frame_size = config.internal_frame_size;

    if (args.rate_given) {
        if (args.rate_arg <= 0) {
            roc_log(LogError, "invalid --rate: should be > 0");
            return 1;
        }
        source_config.sample_rate = (size_t)args.rate_arg;
    } else {
        if (!config.resampling) {
            source_config.sample_rate = pipeline::DefaultSampleRate;
        }
    }

    config.interleaving = args.interleaving_flag;
    config.poisoning = args.poisoning_flag;

    core::BufferPool<uint8_t> byte_buffer_pool(allocator, max_packet_size,
                                               args.poisoning_flag);
    core::BufferPool<audio::sample_t> sample_buffer_pool(
        allocator, config.internal_frame_size, args.poisoning_flag);
    packet::PacketPool packet_pool(allocator, args.poisoning_flag);

    core::UniquePtr<sndio::ISource> source(
        sndio::BackendDispatcher::instance().open_source(allocator, args.driver_arg,
                                                         args.input_arg, source_config),
        allocator);
    if (!source) {
        roc_log(LogError, "can't open input file or device: driver=%s input=%s",
                args.driver_arg, args.input_arg);
        return 1;
    }

    config.timing = !source->has_clock();
    config.input_sample_rate = source->sample_rate();

    fec::CodecMap codec_map;
    rtp::FormatMap format_map;

    netio::Transceiver trx(packet_pool, byte_buffer_pool, allocator);
    if (!trx.valid()) {
        roc_log(LogError, "can't create network transceiver");
        return 1;
    }

    packet::Address local_addr;
    if (source_port.address.version() == 6) {
        local_addr.set_host_ipv6("::", 0);
    } else {
        local_addr.set_host_ipv4("0.0.0.0", 0);
    }
    if (!local_addr.valid()) {
        roc_panic("can't initialize local address");
    }

    packet::IWriter* udp_sender = trx.add_udp_sender(local_addr);
    if (!udp_sender) {
        roc_log(LogError, "can't create udp sender");
        return 1;
    }

    pipeline::Sender sender(config, source_port, *udp_sender, repair_port, *udp_sender,
                            codec_map, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);
    if (!sender.valid()) {
        roc_log(LogError, "can't create sender pipeline");
        return 1;
    }

    sndio::Pump pump(sample_buffer_pool, *source, sender, config.internal_frame_size,
                     sndio::Pump::ModePermanent);
    if (!pump.valid()) {
        roc_log(LogError, "can't create audio pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
