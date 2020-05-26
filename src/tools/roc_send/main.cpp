/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri.h"
#include "roc_address/io_uri.h"
#include "roc_address/protocol_map.h"
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
#include "roc_peer/sender.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/print_supported.h"
#include "roc_sndio/pump.h"

#include "roc_send/cmdline.h"

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

    pipeline::SenderConfig sender_config;

    if (args.packet_length_given) {
        if (!core::parse_duration(args.packet_length_arg, sender_config.packet_length)) {
            roc_log(LogError, "invalid --packet-length");
            return 1;
        }
    }

    if (args.frame_length_given) {
        if (!core::parse_duration(args.frame_length_arg,
                                  sender_config.internal_frame_length)) {
            roc_log(LogError, "invalid --frame-length: bad format");
            return 1;
        }
        if (packet::ns_to_size(sender_config.internal_frame_length,
                               sender_config.input_sample_rate,
                               sender_config.input_channels)
            <= 0) {
            roc_log(LogError, "invalid --frame-length: should be > 0");
            return 1;
        }
    }

    sndio::BackendDispatcher::instance().set_frame_size(
        sender_config.internal_frame_length, sender_config.input_sample_rate,
        sender_config.input_channels);

    address::EndpointURI source_endpoint(context.allocator());
    if (args.source_given) {
        if (!address::parse_endpoint_uri(
                args.source_arg, address::EndpointURI::Subset_Full, source_endpoint)) {
            roc_log(LogError, "can't parse remote source endpoint: %s", args.source_arg);
            return 1;
        }
    }

    address::EndpointURI repair_endpoint(context.allocator());
    if (args.repair_given) {
        if (!address::parse_endpoint_uri(
                args.repair_arg, address::EndpointURI::Subset_Full, repair_endpoint)) {
            roc_log(LogError, "can't parse remote repair endpoint: %s", args.repair_arg);
            return 1;
        }
    }

    const address::ProtocolAttrs* source_attrs =
        address::ProtocolMap::instance().find_proto(source_endpoint.proto());
    if (source_attrs) {
        sender_config.fec_encoder.scheme = source_attrs->fec_scheme;
    }

    if (args.nbsrc_given) {
        if (sender_config.fec_encoder.scheme == packet::FEC_None) {
            roc_log(LogError, "--nbsrc can't be used when fec is disabled)");
            return 1;
        }
        if (args.nbsrc_arg <= 0) {
            roc_log(LogError, "invalid --nbsrc: should be > 0");
            return 1;
        }
        sender_config.fec_writer.n_source_packets = (size_t)args.nbsrc_arg;
    }

    if (args.nbrpr_given) {
        if (sender_config.fec_encoder.scheme == packet::FEC_None) {
            roc_log(LogError, "--nbrpr can't be used when fec is disabled");
            return 1;
        }
        if (args.nbrpr_arg <= 0) {
            roc_log(LogError, "invalid --nbrpr: should be > 0");
            return 1;
        }
        sender_config.fec_writer.n_repair_packets = (size_t)args.nbrpr_arg;
    }

    sender_config.resampling = !args.no_resampling_flag;

    switch ((unsigned)args.resampler_backend_arg) {
    case resampler_backend_arg_builtin:
        sender_config.resampler_backend = audio::ResamplerBackend_Builtin;
        break;

    default:
        break;
    }

    switch ((unsigned)args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        sender_config.resampler_profile = audio::ResamplerProfile_Low;
        break;

    case resampler_profile_arg_medium:
        sender_config.resampler_profile = audio::ResamplerProfile_Medium;
        break;

    case resampler_profile_arg_high:
        sender_config.resampler_profile = audio::ResamplerProfile_High;
        break;

    default:
        roc_panic("unexpected resampler profile");
    }

    sender_config.interleaving = args.interleaving_flag;
    sender_config.poisoning = args.poisoning_flag;
    sender_config.profiling = args.profiling_flag;

    sndio::Config io_config;
    io_config.channels = sender_config.input_channels;
    io_config.sample_rate = sender_config.input_sample_rate;
    io_config.frame_length = sender_config.internal_frame_length;

    if (args.rate_given) {
        if (args.rate_arg <= 0) {
            roc_log(LogError, "invalid --rate: should be > 0");
            return 1;
        }
        io_config.sample_rate = (size_t)args.rate_arg;
    } else {
        if (!sender_config.resampling) {
            io_config.sample_rate = pipeline::DefaultSampleRate;
        }
    }

    address::IoURI input_uri(context.allocator());
    if (args.input_given) {
        if (!address::parse_io_uri(args.input_arg, input_uri)) {
            roc_log(LogError, "invalid --input file or device URI");
            return 1;
        }
    }

    if (args.input_format_given) {
        if (input_uri.is_valid() && !input_uri.is_file()) {
            roc_log(LogError,
                    "--input-format can't be used if --input is not a file URI");
            return 1;
        }
    } else {
        if (input_uri.is_special_file()) {
            roc_log(LogError, "--input-format should be specified if --input is \"-\"");
            return 1;
        }
    }

    core::ScopedPtr<sndio::ISource> input_source(
        sndio::BackendDispatcher::instance().open_source(
            context.allocator(), input_uri, args.input_format_arg, io_config),
        context.allocator());
    if (!input_source) {
        roc_log(LogError, "can't open input file or device: uri=%s format=%s",
                args.input_arg, args.input_format_arg);
        return 1;
    }

    sender_config.timing = !input_source->has_clock();
    sender_config.input_sample_rate = input_source->sample_rate();

    peer::Sender sender(context, sender_config);
    if (!sender.valid()) {
        roc_log(LogError, "can't create sender peer");
        return 1;
    }

    if (args.source_given) {
        if (args.broadcast_given) {
            if (!sender.set_broadcast_enabled(address::Iface_AudioSource, true)) {
                roc_log(LogError, "can't enable broadcast");
                return 1;
            }
        }
        if (!sender.connect(address::Iface_AudioSource, source_endpoint)) {
            roc_log(LogError, "can't connect sender to source endpoint");
            return 1;
        }
    }

    if (args.repair_given) {
        if (args.broadcast_given) {
            if (!sender.set_broadcast_enabled(address::Iface_AudioRepair, true)) {
                roc_log(LogError, "can't enable broadcast");
                return 1;
            }
        }
        if (!sender.connect(address::Iface_AudioRepair, repair_endpoint)) {
            roc_log(LogError, "can't connect sender to repair endpoint");
            return 1;
        }
    }

    sndio::Pump pump(context.sample_buffer_pool(), *input_source, NULL, sender.sink(),
                     sender_config.internal_frame_length, sender_config.input_sample_rate,
                     sender_config.input_channels, sndio::Pump::ModePermanent);
    if (!pump.valid()) {
        roc_log(LogError, "can't create audio pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
