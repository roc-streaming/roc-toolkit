/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri.h"
#include "roc_address/io_uri.h"
#include "roc_address/protocol_map.h"
#include "roc_core/crash_handler.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"
#include "roc_core/parse_units.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/scoped_release.h"
#include "roc_core/time.h"
#include "roc_dbgio/print_supported.h"
#include "roc_netio/network_loop.h"
#include "roc_node/context.h"
#include "roc_node/sender.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/io_pump.h"
#include "roc_status/code_to_str.h"

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

    pipeline::SenderSinkConfig sender_config;

    sndio::IoConfig io_config;

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

    if (args.io_encoding_given) {
        if (!audio::parse_sample_spec(args.io_encoding_arg, io_config.sample_spec)) {
            roc_log(LogError, "invalid --io-encoding");
            return 1;
        }
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
            roc_log(LogError,
                    "--nbsrc can't be used when --source protocol doesn't support fec)");
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
            roc_log(LogError,
                    "--nbrpr can't be used when --source protocol doesn't support fec");
            return 1;
        }
        if (args.nbrpr_arg <= 0) {
            roc_log(LogError, "invalid --nbrpr: should be > 0");
            return 1;
        }
        sender_config.fec_writer.n_repair_packets = (size_t)args.nbrpr_arg;
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
    case resampler_backend_arg_auto:
        sender_config.resampler.backend = audio::ResamplerBackend_Auto;
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

    if (args.target_latency_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--target-latency can be specified only"
                    " when --latency-profile is not 'intact'");
            return 1;
        }
        if (strcmp(args.target_latency_arg, "auto") == 0) {
            sender_config.latency.target_latency = 0;
        } else {
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
    }

    if (args.latency_tolerance_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--latency-tolerance can be specified only"
                    " when --latency-profile is not 'intact'");
            return 1;
        }
        if (!core::parse_duration(args.latency_tolerance_arg,
                                  sender_config.latency.latency_tolerance)) {
            roc_log(LogError, "invalid --latency-tolerance: bad format");
            return 1;
        }
        if (sender_config.latency.latency_tolerance <= 0) {
            roc_log(LogError, "invalid --latency-tolerance: should be > 0");
            return 1;
        }
    }

    if (args.start_latency_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--start-latency can be specified only"
                    " when --latency-profile is not 'intact'");
            return 1;
        }
        if (sender_config.latency.target_latency != 0) {
            roc_log(
                LogError,
                "--start-latency can be specified only in"
                " adaptive latency mode (i.e. --target-latency is 'auto' or omitted)");
            return 1;
        }
        if (!core::parse_duration(args.start_latency_arg,
                                  sender_config.latency.start_target_latency)) {
            roc_log(LogError, "invalid --start-latency: bad format");
            return 1;
        }
        if (sender_config.latency.start_target_latency <= 0) {
            roc_log(LogError, "invalid --start-latency: should be > 0");
            return 1;
        }
    }

    if (args.min_latency_given || args.max_latency_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--min-latency and --max-latency can be specified only"
                    " when --latency-profile is not 'intact'");
            return 1;
        }
        if (sender_config.latency.target_latency != 0) {
            roc_log(
                LogError,
                "--min-latency and --max-latency can be specified only in"
                " adaptive latency mode (i.e. --target-latency is 'auto' or omitted)");
            return 1;
        }
        if (!args.min_latency_given || !args.max_latency_given) {
            roc_log(LogError,
                    "--min-latency and --max-latency should be specified together");
            return 1;
        }
        if (!core::parse_duration(args.min_latency_arg,
                                  sender_config.latency.min_target_latency)) {
            roc_log(LogError, "invalid --min-latency: bad format");
            return 1;
        }
        if (sender_config.latency.min_target_latency <= 0) {
            roc_log(LogError, "invalid --min-latency: should be > 0");
            return 1;
        }
        if (!core::parse_duration(args.max_latency_arg,
                                  sender_config.latency.max_target_latency)) {
            roc_log(LogError, "invalid --max-latency: bad format");
            return 1;
        }
        if (sender_config.latency.max_target_latency <= 0) {
            roc_log(LogError, "invalid --max-latency: should be > 0");
            return 1;
        }
        if (sender_config.latency.min_target_latency
            > sender_config.latency.max_target_latency) {
            roc_log(
                LogError,
                "incorrect --max-latency: should be greater or equal to --min-latency");
            return 1;
        }
    }

    sender_config.enable_profiling = args.profile_flag;

    if (args.dump_given) {
        sender_config.dumper.dump_file = args.dump_arg;
    }

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
    if (context.init_status() != status::StatusOK) {
        roc_log(LogError, "can't initialize node context: status=%s",
                status::code_to_str(context.init_status()));
        return 1;
    }

    sndio::BackendDispatcher backend_dispatcher(
        context.frame_pool(), context.frame_buffer_pool(), context.arena());

    if (args.list_supported_given) {
        if (!dbgio::print_supported(dbgio::Print_Netio | dbgio::Print_Sndio
                                        | dbgio::Print_Audio | dbgio::Print_FEC,
                                    backend_dispatcher, context.arena())) {
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
        const status::StatusCode code = backend_dispatcher.open_source(
            input_uri, args.input_format_arg, io_config, input_source);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open --input file or device: status=%s",
                    status::code_to_str(code));
            return 1;
        }
    } else {
        const status::StatusCode code =
            backend_dispatcher.open_default_source(io_config, input_source);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open default --input device: status=%s",
                    status::code_to_str(code));
            return 1;
        }
    }

    sender_config.enable_cpu_clock = !input_source->has_clock();
    sender_config.input_sample_spec = input_source->sample_spec();

    if (!sender_config.input_sample_spec.is_valid()) {
        roc_log(LogError,
                "can't detect input encoding, try to set it "
                "explicitly with --rate option");
        return 1;
    }

    node::Sender sender(context, sender_config);
    if (sender.init_status() != status::StatusOK) {
        roc_log(LogError, "can't create sender node: status=%s",
                status::code_to_str(sender.init_status()));
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

    if (sender.has_incomplete_slots()) {
        roc_log(
            LogError,
            "incomplete sender configuration:"
            " FEC is implied by protocol, but matching --source or --repair is missing");
        return 1;
    }

    sndio::IoConfig pump_config;
    pump_config.sample_spec = input_source->sample_spec();
    pump_config.frame_length = io_config.frame_length;

    sndio::IoPump pump(context.frame_pool(), context.frame_buffer_pool(), *input_source,
                       NULL, sender.sink(), pump_config, sndio::IoPump::ModePermanent);
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
