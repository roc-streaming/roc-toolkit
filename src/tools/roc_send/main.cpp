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
#include "roc_core/time.h"
#include "roc_netio/network_loop.h"
#include "roc_node/context.h"
#include "roc_node/sender.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/print_supported.h"
#include "roc_sndio/pump.h"

#include "roc_send/cmdline.h"

using namespace roc;

int main(int argc, char** argv) {
    core::HeapArena::set_guards(core::HeapArena_DefaultGuards
                                | core::HeapArena_LeakGuard);

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

    pipeline::SenderSinkConfig sender_config;

    sndio::Config io_config;

    if (args.frame_len_given) {
        if (!core::parse_duration(args.frame_len_arg, io_config.frame_length)) {
            roc_log(LogError, "invalid --frame-len: bad format");
            return 1;
        }
        if (io_config.frame_length <= 0) {
            roc_log(LogError, "invalid --frame-len: should be > 0");
            return 1;
        }
    }

    if (args.io_latency_given) {
        if (!core::parse_duration(args.io_latency_arg, io_config.latency)) {
            roc_log(LogError, "invalid --io-latency: bad format");
            return 1;
        }
        if (io_config.latency <= 0) {
            roc_log(LogError, "invalid --io-latency: should be > 0");
            return 1;
        }
    }

    // TODO(gh-608): replace --rate with --io-encoding
    if (args.rate_given) {
        if (args.rate_arg <= 0) {
            roc_log(LogError, "invalid --rate: should be > 0");
            return 1;
        }
        io_config.sample_spec.set_sample_rate((size_t)args.rate_arg);
    }

    // TODO(gh-568): remove set_frame_size() after removing sox
    sndio::BackendMap::instance().set_frame_size(io_config.frame_length,
                                                 sender_config.input_sample_spec);

    if (args.packet_len_given) {
        if (!core::parse_duration(args.packet_len_arg, sender_config.packet_length)) {
            roc_log(LogError, "invalid --packet-len: bad format");
            return 1;
        }
        if (sender_config.packet_length <= 0) {
            roc_log(LogError, "invalid --packet-len: should be > 0");
            return 1;
        }
    }

    if (args.source_given) {
        address::EndpointUri source_endpoint(heap_arena);
        if (!address::parse_endpoint_uri(
                args.source_arg[0], address::EndpointUri::Subset_Full, source_endpoint)) {
            roc_log(LogError, "can't parse --source endpoint: %s", args.source_arg[0]);
            return 1;
        }

        const address::ProtocolAttrs* source_attrs =
            address::ProtocolMap::instance().find_by_id(source_endpoint.proto());
        if (source_attrs) {
            sender_config.fec_encoder.scheme = source_attrs->fec_scheme;
        }
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

    if (args.target_latency_given) {
        if (!core::parse_duration(args.target_latency_arg,
                                  sender_config.latency.target_latency)) {
            roc_log(LogError, "invalid --target-latency: bad format");
            return 1;
        }
        if (sender_config.latency.target_latency <= 0) {
            roc_log(LogError, "invalid --target-latency: should be > 0");
            return 1;
        }
    }

    if (args.min_latency_given || args.max_latency_given) {
        if (!args.min_latency_given || !args.max_latency_given) {
            roc_log(LogError,
                    "--min-latency and --max-latency should be specified together");
            return 1;
        }

        if (!core::parse_duration(args.min_latency_arg,
                                  sender_config.latency.min_latency)) {
            roc_log(LogError, "invalid --min-latency: bad format");
            return 1;
        }

        if (!core::parse_duration(args.max_latency_arg,
                                  sender_config.latency.max_latency)) {
            roc_log(LogError, "invalid --max-latency: bad format");
            return 1;
        }
        if (sender_config.latency.max_latency <= 0) {
            roc_log(LogError, "invalid --max-latency: should be > 0");
            return 1;
        }
    }

    switch (args.latency_backend_arg) {
    case latency_backend_arg_niq:
        sender_config.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        break;
    default:
        break;
    }

    switch (args.latency_profile_arg) {
    case latency_profile_arg_responsive:
        sender_config.latency.tuner_profile = audio::LatencyTunerProfile_Responsive;
        break;
    case latency_profile_arg_gradual:
        sender_config.latency.tuner_profile = audio::LatencyTunerProfile_Gradual;
        break;
    case latency_profile_arg_intact:
        sender_config.latency.tuner_profile = audio::LatencyTunerProfile_Intact;
        break;
    default:
        break;
    }

    switch (args.resampler_backend_arg) {
    case resampler_backend_arg_default:
        sender_config.resampler.backend = audio::ResamplerBackend_Default;
        break;
    case resampler_backend_arg_builtin:
        sender_config.resampler.backend = audio::ResamplerBackend_Builtin;
        break;
    case resampler_backend_arg_speex:
        sender_config.resampler.backend = audio::ResamplerBackend_Speex;
        break;
    case resampler_backend_arg_speexdec:
        sender_config.resampler.backend = audio::ResamplerBackend_SpeexDec;
        break;
    default:
        break;
    }

    switch (args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        sender_config.resampler.profile = audio::ResamplerProfile_Low;
        break;
    case resampler_profile_arg_medium:
        sender_config.resampler.profile = audio::ResamplerProfile_Medium;
        break;
    case resampler_profile_arg_high:
        sender_config.resampler.profile = audio::ResamplerProfile_High;
        break;
    default:
        break;
    }

    sender_config.enable_interleaving = args.interleaving_flag;
    sender_config.enable_profiling = args.profiling_flag;

    node::ContextConfig context_config;

    if (args.max_packet_size_given) {
        if (!core::parse_size(args.max_packet_size_arg, context_config.max_packet_size)) {
            roc_log(LogError, "invalid --max-packet-size: bad format");
            return 1;
        }
        if (context_config.max_packet_size == 0) {
            roc_log(LogError, "invalid --max-packet-size: should be > 0");
            return 1;
        }
    } else {
        audio::SampleSpec spec = io_config.sample_spec;
        spec.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                          audio::ChanOrder_Smpte, audio::ChanMask_Surround_7_1_4, 48000);
        context_config.max_packet_size = packet::Packet::approx_size(
            spec.ns_2_samples_overall(io_config.frame_length));
    }

    if (args.max_frame_size_given) {
        if (!core::parse_size(args.max_frame_size_arg, context_config.max_frame_size)) {
            roc_log(LogError, "invalid --max-frame-size: bad format");
            return 1;
        }
        if (context_config.max_frame_size == 0) {
            roc_log(LogError, "invalid --max-frame-size: should be > 0");
            return 1;
        }
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

    address::IoUri input_uri(context.arena());
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

    core::ScopedPtr<sndio::ISource> input_source;
    if (input_uri.is_valid()) {
        input_source.reset(
            backend_dispatcher.open_source(input_uri, args.input_format_arg, io_config),
            context.arena());
    } else {
        input_source.reset(backend_dispatcher.open_default_source(io_config),
                           context.arena());
    }
    if (!input_source) {
        roc_log(LogError, "can't open input file or device: uri=%s format=%s",
                args.input_arg, args.input_format_arg);
        return 1;
    }

    sender_config.enable_timing = !input_source->has_clock();
    sender_config.input_sample_spec = input_source->sample_spec();

    if (!sender_config.input_sample_spec.is_valid()) {
        roc_log(LogError,
                "can't detect input encoding, try to set it "
                "explicitly with --rate option");
        return 1;
    }

    node::Sender sender(context, sender_config);
    if (!sender.is_valid()) {
        roc_log(LogError, "can't create sender node");
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

    for (size_t slot = 0; slot < (size_t)args.source_given; slot++) {
        address::EndpointUri source_endpoint(context.arena());
        if (!address::parse_endpoint_uri(args.source_arg[slot],
                                         address::EndpointUri::Subset_Full,
                                         source_endpoint)) {
            roc_log(LogError, "can't parse --source endpoint: %s", args.source_arg[slot]);
            return 1;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (!sender.configure(slot, address::Iface_AudioSource, iface_config)) {
            roc_log(LogError, "can't configure --source endpoint");
            return 1;
        }

        if (!sender.connect(slot, address::Iface_AudioSource, source_endpoint)) {
            roc_log(LogError, "can't connect sender to source endpoint");
            return 1;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.repair_given; slot++) {
        address::EndpointUri repair_endpoint(context.arena());
        if (!address::parse_endpoint_uri(args.repair_arg[slot],
                                         address::EndpointUri::Subset_Full,
                                         repair_endpoint)) {
            roc_log(LogError, "can't parse --repair endpoint: %s", args.repair_arg[slot]);
            return 1;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (!sender.configure(slot, address::Iface_AudioRepair, iface_config)) {
            roc_log(LogError, "can't configure --repair endpoint");
            return 1;
        }

        if (!sender.connect(slot, address::Iface_AudioRepair, repair_endpoint)) {
            roc_log(LogError, "can't connect sender to repair endpoint");
            return 1;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.control_given; slot++) {
        address::EndpointUri control_endpoint(context.arena());
        if (!address::parse_endpoint_uri(args.control_arg[slot],
                                         address::EndpointUri::Subset_Full,
                                         control_endpoint)) {
            roc_log(LogError, "can't parse --control endpoint: %s",
                    args.control_arg[slot]);
            return 1;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (!sender.configure(slot, address::Iface_AudioControl, iface_config)) {
            roc_log(LogError, "can't configure --control endpoint");
            return 1;
        }

        if (!sender.connect(slot, address::Iface_AudioControl, control_endpoint)) {
            roc_log(LogError, "can't connect sender to control endpoint");
            return 1;
        }
    }

    if (sender.has_incomplete()) {
        roc_log(
            LogError,
            "incomplete sender configuration:"
            " FEC is implied by protocol, but matching --source or --repair is missing");
        return 1;
    }

    sndio::Pump pump(context.frame_buffer_pool(), *input_source, NULL, sender.sink(),
                     io_config.frame_length, sender_config.input_sample_spec,
                     sndio::Pump::ModePermanent);
    if (!pump.is_valid()) {
        roc_log(LogError, "can't create audio pump");
        return 1;
    }

    const bool ok = pump.run();

    return ok ? 0 : 1;
}
