/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_address/network_uri.h"
#include "roc_address/protocol_map.h"
#include "roc_core/crash_handler.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"
#include "roc_core/parse_units.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/scoped_release.h"
#include "roc_dbgio/print_supported.h"
#include "roc_node/context.h"
#include "roc_node/sender.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/io_pump.h"
#include "roc_status/code_to_str.h"

#include "roc_send/cmdline.h"

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

bool build_io_config(const gengetopt_args_info& args, sndio::IoConfig& io_config) {
    if (args.io_encoding_given) {
        if (!audio::parse_sample_spec(args.io_encoding_arg, io_config.sample_spec)) {
            roc_log(LogError, "invalid --io-encoding");
            return false;
        }
    }

    if (args.io_latency_given) {
        if (!core::parse_duration(args.io_latency_arg, io_config.latency)) {
            roc_log(LogError, "invalid --io-latency: bad format");
            return false;
        }
        if (io_config.latency <= 0) {
            roc_log(LogError, "invalid --io-latency: should be > 0");
            return false;
        }
    }

    if (args.io_frame_len_given) {
        if (!core::parse_duration(args.io_frame_len_arg, io_config.frame_length)) {
            roc_log(LogError, "invalid --frame-len: bad format");
            return false;
        }
        if (io_config.frame_length <= 0) {
            roc_log(LogError, "invalid --frame-len: should be > 0");
            return false;
        }
    }

    return true;
}

bool build_context_config(const gengetopt_args_info& args,
                          const sndio::IoConfig& io_config,
                          node::ContextConfig& context_config) {
    if (args.max_packet_size_given) {
        if (!core::parse_size(args.max_packet_size_arg, context_config.max_packet_size)) {
            roc_log(LogError, "invalid --max-packet-size: bad format");
            return false;
        }
        if (context_config.max_packet_size == 0) {
            roc_log(LogError, "invalid --max-packet-size: should be > 0");
            return false;
        }
    } else {
        audio::SampleSpec spec = io_config.sample_spec;
        spec.use_defaults(audio::Format_Pcm, audio::PcmSubformat_Raw,
                          audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                          audio::ChanMask_Surround_7_1_4, 48000);
        core::nanoseconds_t len = io_config.frame_length;
        if (len == 0) {
            len = 10 * core::Millisecond;
        }
        context_config.max_packet_size =
            packet::Packet::approx_size(spec.ns_2_samples_overall(len));
    }

    if (args.max_frame_size_given) {
        if (!core::parse_size(args.max_frame_size_arg, context_config.max_frame_size)) {
            roc_log(LogError, "invalid --max-frame-size: bad format");
            return false;
        }
        if (context_config.max_frame_size == 0) {
            roc_log(LogError, "invalid --max-frame-size: should be > 0");
            return false;
        }
    }

    return true;
}

bool build_sender_config(const gengetopt_args_info& args,
                         pipeline::SenderSinkConfig& sender_config,
                         node::Context& context,
                         sndio::ISource& input_source) {
    if (args.packet_encoding_given) {
        rtp::Encoding encoding;
        if (!rtp::parse_encoding(args.packet_encoding_arg, encoding)) {
            roc_log(LogError, "invalid --packet-encoding");
            return false;
        }

        const status::StatusCode code =
            context.encoding_map().register_encoding(encoding);
        if (code != status::StatusOK) {
            roc_log(LogError, "can't register packet encoding: status=%s",
                    status::code_to_str(code));
            return false;
        }

        sender_config.payload_type = encoding.payload_type;
    }

    if (args.packet_len_given) {
        if (!core::parse_duration(args.packet_len_arg, sender_config.packet_length)) {
            roc_log(LogError, "invalid --packet-len: bad format");
            return false;
        }
        if (sender_config.packet_length <= 0) {
            roc_log(LogError, "invalid --packet-len: should be > 0");
            return false;
        }
    }

    if (args.fec_encoding_given && strcmp(args.fec_encoding_arg, "none") == 0) {
        sender_config.fec_encoder.scheme = packet::FEC_None;
    } else if (args.fec_encoding_given && strcmp(args.fec_encoding_arg, "auto") != 0) {
        sender_config.fec_encoder.scheme =
            packet::fec_scheme_from_str(args.fec_encoding_arg);

        if (sender_config.fec_encoder.scheme == packet::FEC_None) {
            roc_log(LogError, "invalid --fec-encoding");
            return false;
        }
    } else if (args.source_given) {
        address::NetworkUri source_endpoint(context.arena());
        if (!address::parse_network_uri(
                args.source_arg[0], address::NetworkUri::Subset_Full, source_endpoint)) {
            roc_log(LogError, "can't parse --source endpoint: %s", args.source_arg[0]);
            return false;
        }
        const address::ProtocolAttrs* source_attrs =
            address::ProtocolMap::instance().find_by_id(source_endpoint.proto());
        if (source_attrs) {
            sender_config.fec_encoder.scheme = source_attrs->fec_scheme;
        }
    }

    if (args.fec_block_src_given || args.fec_block_rpr_given) {
        if (sender_config.fec_encoder.scheme == packet::FEC_None) {
            roc_log(LogError,
                    "--fec-block-src and --fec-block-rpr can't be used when"
                    " --source protocol doesn't support fec");
            return false;
        }
        if (!args.fec_block_src_given || !args.fec_block_rpr_given) {
            roc_log(LogError,
                    "--fec-block-src and --fec-block-rpr should be specified together");
            return false;
        }
        if (args.fec_block_src_arg <= 0) {
            roc_log(LogError, "invalid --fec-block-src: should be > 0");
            return false;
        }
        if (args.fec_block_rpr_arg <= 0) {
            roc_log(LogError, "invalid --fec-block-rpr: should be > 0");
            return false;
        }
        sender_config.fec_writer.n_source_packets = (size_t)args.fec_block_src_arg;
        sender_config.fec_writer.n_repair_packets = (size_t)args.fec_block_rpr_arg;
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

    if (args.target_latency_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--target-latency can be specified only"
                    " when --latency-profile is not 'intact'");
            return false;
        }
        if (strcmp(args.target_latency_arg, "auto") == 0) {
            sender_config.latency.target_latency = 0;
        } else {
            if (!core::parse_duration(args.target_latency_arg,
                                      sender_config.latency.target_latency)) {
                roc_log(LogError, "invalid --target-latency: bad format");
                return false;
            }
            if (sender_config.latency.target_latency <= 0) {
                roc_log(LogError, "invalid --target-latency: should be > 0");
                return false;
            }
        }
    }

    if (args.latency_tolerance_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--latency-tolerance can be specified only"
                    " when --latency-profile is not 'intact'");
            return false;
        }
        if (!core::parse_duration(args.latency_tolerance_arg,
                                  sender_config.latency.latency_tolerance)) {
            roc_log(LogError, "invalid --latency-tolerance: bad format");
            return false;
        }
        if (sender_config.latency.latency_tolerance <= 0) {
            roc_log(LogError, "invalid --latency-tolerance: should be > 0");
            return false;
        }
    }

    if (args.start_latency_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--start-latency can be specified only"
                    " when --latency-profile is not 'intact'");
            return false;
        }
        if (sender_config.latency.target_latency != 0) {
            roc_log(
                LogError,
                "--start-latency can be specified only in"
                " adaptive latency mode (i.e. --target-latency is 'auto' or omitted)");
            return false;
        }
        if (!core::parse_duration(args.start_latency_arg,
                                  sender_config.latency.start_target_latency)) {
            roc_log(LogError, "invalid --start-latency: bad format");
            return false;
        }
        if (sender_config.latency.start_target_latency <= 0) {
            roc_log(LogError, "invalid --start-latency: should be > 0");
            return false;
        }
    }

    if (args.min_latency_given || args.max_latency_given) {
        if (sender_config.latency.tuner_profile == audio::LatencyTunerProfile_Intact) {
            roc_log(LogError,
                    "--min-latency and --max-latency can be specified only"
                    " when --latency-profile is not 'intact'");
            return false;
        }
        if (sender_config.latency.target_latency != 0) {
            roc_log(
                LogError,
                "--min-latency and --max-latency can be specified only in"
                " adaptive latency mode (i.e. --target-latency is 'auto' or omitted)");
            return false;
        }
        if (!args.min_latency_given || !args.max_latency_given) {
            roc_log(LogError,
                    "--min-latency and --max-latency should be specified together");
            return false;
        }
        if (!core::parse_duration(args.min_latency_arg,
                                  sender_config.latency.min_target_latency)) {
            roc_log(LogError, "invalid --min-latency: bad format");
            return false;
        }
        if (sender_config.latency.min_target_latency <= 0) {
            roc_log(LogError, "invalid --min-latency: should be > 0");
            return false;
        }
        if (!core::parse_duration(args.max_latency_arg,
                                  sender_config.latency.max_target_latency)) {
            roc_log(LogError, "invalid --max-latency: bad format");
            return false;
        }
        if (sender_config.latency.max_target_latency <= 0) {
            roc_log(LogError, "invalid --max-latency: should be > 0");
            return false;
        }
    }

    sender_config.enable_profiling = args.prof_flag;

    if (args.dump_given) {
        sender_config.dumper.dump_file = args.dump_arg;
    }

    sender_config.enable_cpu_clock = !input_source.has_clock();
    sender_config.input_sample_spec = input_source.sample_spec();

    if (!sender_config.input_sample_spec.is_complete()) {
        roc_log(LogError,
                "can't detect input encoding, try to set it "
                "explicitly with --io-encoding option");
        return false;
    }

    return true;
}

bool parse_input_uri(const gengetopt_args_info& args, address::IoUri& input_uri) {
    if (args.input_given) {
        if (!address::parse_io_uri(args.input_arg, input_uri)) {
            roc_log(LogError, "invalid --input file or device URI");
            return false;
        }
    }

    return true;
}

bool open_input_source(sndio::BackendDispatcher& backend_dispatcher,
                       const sndio::IoConfig& io_config,
                       const address::IoUri& input_uri,
                       core::ScopedPtr<sndio::ISource>& input_source) {
    if (input_uri.is_valid()) {
        const status::StatusCode code =
            backend_dispatcher.open_source(input_uri, io_config, input_source);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open --input file or device: status=%s",
                    status::code_to_str(code));
            return false;
        }
    } else {
        const status::StatusCode code =
            backend_dispatcher.open_default_source(io_config, input_source);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open default --input device: status=%s",
                    status::code_to_str(code));
            return false;
        }
    }

    return true;
}

bool prepare_sender(const gengetopt_args_info& args,
                    node::Context& context,
                    node::Sender& sender) {
    if (args.source_given == 0) {
        roc_log(LogError, "at least one --source endpoint should be specified");
        return false;
    }

    if (args.repair_given != 0 && args.repair_given != args.source_given) {
        roc_log(LogError,
                "invalid number of --repair endpoints: expected either 0 or %d endpoints"
                " (one per --source), got %d endpoints",
                (int)args.source_given, (int)args.repair_given);
        return false;
    }

    if (args.control_given != 0 && args.control_given != args.source_given) {
        roc_log(LogError,
                "invalid number of --control endpoints: expected either 0 or %d endpoints"
                " (one per --source), got %d endpoints",
                (int)args.source_given, (int)args.control_given);
        return false;
    }

    if (args.miface_given != 0 && args.miface_given != args.source_given) {
        roc_log(LogError,
                "invalid number of --miface values: expected either 0 or %d values"
                " (one per --source), got %d values",
                (int)args.source_given, (int)args.miface_given);
        return false;
    }

    for (size_t slot = 0; slot < (size_t)args.source_given; slot++) {
        address::NetworkUri source_endpoint(context.arena());
        if (!address::parse_network_uri(args.source_arg[slot],
                                        address::NetworkUri::Subset_Full,
                                        source_endpoint)) {
            roc_log(LogError, "can't parse --source endpoint: %s", args.source_arg[slot]);
            return false;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (args.miface_given) {
            if (strlen(args.miface_arg[slot])
                >= sizeof(iface_config.multicast_interface)) {
                roc_log(LogError, "invalid --miface \"%s\": string too long",
                        args.miface_arg[slot]);
                return false;
            }
            strcpy(iface_config.multicast_interface, args.miface_arg[slot]);
        }

        if (!sender.configure(slot, address::Iface_AudioSource, iface_config)) {
            roc_log(LogError, "can't configure --source endpoint");
            return false;
        }

        if (!sender.connect(slot, address::Iface_AudioSource, source_endpoint)) {
            roc_log(LogError, "can't connect sender to source endpoint");
            return false;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.repair_given; slot++) {
        address::NetworkUri repair_endpoint(context.arena());
        if (!address::parse_network_uri(args.repair_arg[slot],
                                        address::NetworkUri::Subset_Full,
                                        repair_endpoint)) {
            roc_log(LogError, "can't parse --repair endpoint: %s", args.repair_arg[slot]);
            return false;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (!sender.configure(slot, address::Iface_AudioRepair, iface_config)) {
            roc_log(LogError, "can't configure --repair endpoint");
            return false;
        }

        if (!sender.connect(slot, address::Iface_AudioRepair, repair_endpoint)) {
            roc_log(LogError, "can't connect sender to repair endpoint");
            return false;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.control_given; slot++) {
        address::NetworkUri control_endpoint(context.arena());
        if (!address::parse_network_uri(args.control_arg[slot],
                                        address::NetworkUri::Subset_Full,
                                        control_endpoint)) {
            roc_log(LogError, "can't parse --control endpoint: %s",
                    args.control_arg[slot]);
            return false;
        }

        netio::UdpConfig iface_config;
        iface_config.enable_reuseaddr = args.reuseaddr_given;

        if (!sender.configure(slot, address::Iface_AudioControl, iface_config)) {
            roc_log(LogError, "can't configure --control endpoint");
            return false;
        }

        if (!sender.connect(slot, address::Iface_AudioControl, control_endpoint)) {
            roc_log(LogError, "can't connect sender to control endpoint");
            return false;
        }
    }

    if (sender.has_incomplete_slots()) {
        roc_log(
            LogError,
            "incomplete sender configuration:"
            " FEC is implied by protocol, but matching --source or --repair is missing");
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

    sndio::IoConfig io_config;
    if (!build_io_config(args, io_config)) {
        return 1;
    }

    node::ContextConfig context_config;
    if (!build_context_config(args, io_config, context_config)) {
        return 1;
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
    if (!parse_input_uri(args, input_uri)) {
        return 1;
    }

    core::ScopedPtr<sndio::ISource> input_source;
    if (!open_input_source(backend_dispatcher, io_config, input_uri, input_source)) {
        return 1;
    }

    io_config.sample_spec = input_source->sample_spec();
    io_config.frame_length = input_source->frame_length();

    pipeline::SenderSinkConfig sender_config;
    if (!build_sender_config(args, sender_config, context, *input_source)) {
        return 1;
    }

    node::Sender sender(context, sender_config);
    if (sender.init_status() != status::StatusOK) {
        roc_log(LogError, "can't create sender node: status=%s",
                status::code_to_str(sender.init_status()));
        return 1;
    }

    if (!prepare_sender(args, context, sender)) {
        return 1;
    }

    sndio::IoPump pump(context.frame_pool(), context.frame_buffer_pool(), *input_source,
                       NULL, sender.sink(), io_config, sndio::IoPump::ModePermanent);
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
