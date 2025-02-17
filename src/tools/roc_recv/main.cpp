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
#include "roc_node/receiver.h"
#include "roc_pipeline/transcoder_source.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/io_pump.h"
#include "roc_status/code_to_str.h"

#include "roc_recv/cmdline.h"

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
    } else {
        audio::SampleSpec spec = io_config.sample_spec;
        spec.use_defaults(audio::Format_Pcm, audio::PcmSubformat_Raw,
                          audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                          audio::ChanMask_Surround_7_1_4, 48000);
        core::nanoseconds_t len = io_config.frame_length;
        if (len == 0) {
            len = 10 * core::Millisecond;
        }
        context_config.max_frame_size =
            spec.ns_2_samples_overall(len) * sizeof(audio::sample_t);
    }

    return true;
}

bool build_receiver_config(const gengetopt_args_info& args,
                           pipeline::ReceiverSourceConfig& receiver_config,
                           node::Context& context,
                           sndio::ISink& output_sink) {
    for (size_t n = 0; n < args.packet_encoding_given; n++) {
        rtp::Encoding encoding;
        if (!rtp::parse_encoding(args.packet_encoding_arg[n], encoding)) {
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
    }

    switch (args.plc_arg) {
    case plc_arg_none:
        receiver_config.session_defaults.plc.backend = audio::PlcBackend_None;
        break;
    case plc_arg_beep:
        receiver_config.session_defaults.plc.backend = audio::PlcBackend_Beep;
        break;
    default:
        break;
    }

    switch (args.resampler_backend_arg) {
    case resampler_backend_arg_auto:
        receiver_config.session_defaults.resampler.backend = audio::ResamplerBackend_Auto;
        break;
    case resampler_backend_arg_builtin:
        receiver_config.session_defaults.resampler.backend =
            audio::ResamplerBackend_Builtin;
        break;
    case resampler_backend_arg_speex:
        receiver_config.session_defaults.resampler.backend =
            audio::ResamplerBackend_Speex;
        break;
    case resampler_backend_arg_speexdec:
        receiver_config.session_defaults.resampler.backend =
            audio::ResamplerBackend_SpeexDec;
        break;
    default:
        break;
    }

    switch (args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        receiver_config.session_defaults.resampler.profile = audio::ResamplerProfile_Low;
        break;
    case resampler_profile_arg_medium:
        receiver_config.session_defaults.resampler.profile =
            audio::ResamplerProfile_Medium;
        break;
    case resampler_profile_arg_high:
        receiver_config.session_defaults.resampler.profile = audio::ResamplerProfile_High;
        break;
    default:
        break;
    }

    switch (args.latency_backend_arg) {
    case latency_backend_arg_niq:
        receiver_config.session_defaults.latency.tuner_backend =
            audio::LatencyTunerBackend_Niq;
        break;
    default:
        break;
    }

    switch (args.latency_profile_arg) {
    case latency_profile_arg_auto:
        receiver_config.session_defaults.latency.tuner_profile =
            audio::LatencyTunerProfile_Auto;
        break;
    case latency_profile_arg_responsive:
        receiver_config.session_defaults.latency.tuner_profile =
            audio::LatencyTunerProfile_Responsive;
        break;
    case latency_profile_arg_gradual:
        receiver_config.session_defaults.latency.tuner_profile =
            audio::LatencyTunerProfile_Gradual;
        break;
    case latency_profile_arg_intact:
        receiver_config.session_defaults.latency.tuner_profile =
            audio::LatencyTunerProfile_Intact;
        break;
    default:
        break;
    }

    if (args.target_latency_given) {
        if (strcmp(args.target_latency_arg, "auto") == 0) {
            receiver_config.session_defaults.latency.target_latency = 0;
        } else {
            if (!core::parse_duration(
                    args.target_latency_arg,
                    receiver_config.session_defaults.latency.target_latency)) {
                roc_log(LogError, "invalid --target-latency: bad format");
                return false;
            }
            if (receiver_config.session_defaults.latency.target_latency <= 0) {
                roc_log(LogError, "invalid --target-latency: should be 'auto' or > 0");
                return false;
            }
        }
    }

    if (args.latency_tolerance_given) {
        if (!core::parse_duration(
                args.latency_tolerance_arg,
                receiver_config.session_defaults.latency.latency_tolerance)) {
            roc_log(LogError, "invalid --latency-tolerance: bad format");
            return false;
        }
        if (receiver_config.session_defaults.latency.latency_tolerance <= 0) {
            roc_log(LogError, "invalid --latency-tolerance: should be > 0");
            return false;
        }
    }

    if (args.start_latency_given) {
        if (receiver_config.session_defaults.latency.target_latency != 0) {
            roc_log(
                LogError,
                "--start-latency can be specified only in"
                " adaptive latency mode (i.e. --target-latency is 'auto' or omitted)");
            return false;
        }
        if (!core::parse_duration(
                args.start_latency_arg,
                receiver_config.session_defaults.latency.start_target_latency)) {
            roc_log(LogError, "invalid --start-latency: bad format");
            return false;
        }
        if (receiver_config.session_defaults.latency.start_target_latency <= 0) {
            roc_log(LogError, "invalid --start-latency: should be > 0");
            return false;
        }
    }

    if (args.min_latency_given || args.max_latency_given) {
        if (receiver_config.session_defaults.latency.target_latency != 0) {
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
        if (!core::parse_duration(
                args.min_latency_arg,
                receiver_config.session_defaults.latency.min_target_latency)) {
            roc_log(LogError, "invalid --min-latency: bad format");
            return false;
        }
        if (receiver_config.session_defaults.latency.min_target_latency <= 0) {
            roc_log(LogError, "invalid --min-latency: should be > 0");
            return false;
        }
        if (!core::parse_duration(
                args.max_latency_arg,
                receiver_config.session_defaults.latency.max_target_latency)) {
            roc_log(LogError, "invalid --max-latency: bad format");
            return false;
        }
        if (receiver_config.session_defaults.latency.max_target_latency <= 0) {
            roc_log(LogError, "invalid --max-latency: should be > 0");
            return false;
        }
    }

    if (args.no_play_timeout_given) {
        if (!core::parse_duration(
                args.no_play_timeout_arg,
                receiver_config.session_defaults.watchdog.no_playback_timeout)) {
            roc_log(LogError, "invalid --no-play-timeout: bad format");
            return false;
        }
        if (receiver_config.session_defaults.watchdog.no_playback_timeout <= 0) {
            roc_log(LogError, "invalid --no-play-timeout: should be > 0");
            return false;
        }
    }

    if (args.choppy_play_timeout_given) {
        if (!core::parse_duration(
                args.choppy_play_timeout_arg,
                receiver_config.session_defaults.watchdog.choppy_playback_timeout)) {
            roc_log(LogError, "invalid --choppy-play-timeout: bad format");
            return false;
        }
        if (receiver_config.session_defaults.watchdog.choppy_playback_timeout <= 0) {
            roc_log(LogError, "invalid --choppy-play-timeout: should be > 0");
            return false;
        }
    }

    receiver_config.common.enable_profiling = args.prof_flag;

    if (args.dump_given) {
        receiver_config.common.dumper.dump_file = args.dump_arg;
    }

    receiver_config.common.enable_cpu_clock = !output_sink.has_clock();
    receiver_config.common.output_sample_spec = output_sink.sample_spec();

    if (!receiver_config.common.output_sample_spec.is_complete()) {
        roc_log(LogError,
                "can't detect output encoding, try to set it"
                " explicitly with --io-encoding option");
        return false;
    }

    return true;
}

bool parse_output_uri(const gengetopt_args_info& args, address::IoUri& output_uri) {
    if (args.output_given) {
        if (!address::parse_io_uri(args.output_arg, output_uri)) {
            roc_log(LogError, "invalid --output file or device URI");
            return false;
        }
        if (output_uri.is_special_file()) {
            if (!args.io_encoding_given) {
                roc_log(LogError, "--io-encoding is required when --output is \"-\"");
                return false;
            }
        }
    }

    return true;
}

bool open_output_sink(sndio::BackendDispatcher& backend_dispatcher,
                      const sndio::IoConfig& io_config,
                      const address::IoUri& output_uri,
                      core::ScopedPtr<sndio::ISink>& output_sink) {
    if (output_uri.is_valid()) {
        const status::StatusCode code =
            backend_dispatcher.open_sink(output_uri, io_config, output_sink);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open --output file or device: status=%s",
                    status::code_to_str(code));
            return false;
        }
    } else {
        const status::StatusCode code =
            backend_dispatcher.open_default_sink(io_config, output_sink);

        if (code != status::StatusOK) {
            roc_log(LogError, "can't open default --output device: status=%s",
                    status::code_to_str(code));
            return false;
        }
    }

    return true;
}

bool parse_backup_uri(const gengetopt_args_info& args, address::IoUri& backup_uri) {
    if (!address::parse_io_uri(args.backup_arg, backup_uri)) {
        roc_log(LogError, "invalid --backup URI: bad format");
        return false;
    }

    if (!backup_uri.is_file()) {
        roc_log(LogError, "invalid --backup URI: should be file");
        return false;
    }

    if (backup_uri.is_special_file()) {
        roc_log(LogError, "invalid --backup URI: can't be \"-\"");
        return false;
    }

    return true;
}

bool open_backup_source(sndio::BackendDispatcher& backend_dispatcher,
                        const sndio::IoConfig& io_config,
                        const address::IoUri& backup_uri,
                        core::ScopedPtr<sndio::ISource>& backup_source) {
    const status::StatusCode code =
        backend_dispatcher.open_source(backup_uri, io_config, backup_source);

    if (code != status::StatusOK) {
        roc_log(LogError, "can't open --backup file or device: status=%s",
                status::code_to_str(code));
        return false;
    }

    return true;
}

bool open_backup_transcoder(
    core::ScopedPtr<pipeline::TranscoderSource>& backup_transcoder,
    sndio::ISource& backup_source,
    node::Context& context,
    const pipeline::ReceiverSourceConfig& receiver_config) {
    pipeline::TranscoderConfig transcoder_config;

    transcoder_config.resampler.backend =
        receiver_config.session_defaults.resampler.backend;
    transcoder_config.resampler.profile =
        receiver_config.session_defaults.resampler.profile;

    transcoder_config.input_sample_spec =
        audio::SampleSpec(backup_source.sample_spec().sample_rate(),
                          receiver_config.common.output_sample_spec.pcm_subformat(),
                          receiver_config.common.output_sample_spec.channel_set());
    transcoder_config.output_sample_spec =
        audio::SampleSpec(receiver_config.common.output_sample_spec.sample_rate(),
                          receiver_config.common.output_sample_spec.pcm_subformat(),
                          receiver_config.common.output_sample_spec.channel_set());

    backup_transcoder.reset(new (context.arena()) pipeline::TranscoderSource(
        transcoder_config, backup_source, context.processor_map(), context.frame_pool(),
        context.frame_buffer_pool(), context.arena()));
    if (!backup_transcoder) {
        roc_log(LogError, "can't allocate backup pipeline");
        return false;
    }

    if (backup_transcoder->init_status() != status::StatusOK) {
        roc_log(LogError, "can't create backup pipeline: status=%s",
                status::code_to_str(backup_transcoder->init_status()));
        return false;
    }

    return true;
}

bool prepare_receiver(const gengetopt_args_info& args,
                      node::Context& context,
                      node::Receiver& receiver) {
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
        address::NetworkUri endpoint(context.arena());

        if (!address::parse_network_uri(args.source_arg[slot],
                                        address::NetworkUri::Subset_Full, endpoint)) {
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

        if (!receiver.configure(slot, address::Iface_AudioSource, iface_config)) {
            roc_log(LogError, "can't configure --source endpoint");
            return false;
        }

        if (!receiver.bind(slot, address::Iface_AudioSource, endpoint)) {
            roc_log(LogError, "can't bind --source endpoint: %s", args.source_arg[slot]);
            return false;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.repair_given; slot++) {
        address::NetworkUri endpoint(context.arena());

        if (!address::parse_network_uri(args.repair_arg[slot],
                                        address::NetworkUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --repair endpoint: %s", args.source_arg[slot]);
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

        if (!receiver.configure(slot, address::Iface_AudioRepair, iface_config)) {
            roc_log(LogError, "can't configure --repair endpoint");
            return false;
        }

        if (!receiver.bind(slot, address::Iface_AudioRepair, endpoint)) {
            roc_log(LogError, "can't bind --repair port: %s", args.repair_arg[slot]);
            return false;
        }
    }

    for (size_t slot = 0; slot < (size_t)args.control_given; slot++) {
        address::NetworkUri endpoint(context.arena());

        if (!address::parse_network_uri(args.control_arg[slot],
                                        address::NetworkUri::Subset_Full, endpoint)) {
            roc_log(LogError, "can't parse --control endpoint: %s",
                    args.control_arg[slot]);
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

        if (!receiver.configure(slot, address::Iface_AudioControl, iface_config)) {
            roc_log(LogError, "can't configure --control endpoint");
            return false;
        }

        if (!receiver.bind(slot, address::Iface_AudioControl, endpoint)) {
            roc_log(LogError, "can't bind --control endpoint: %s",
                    args.control_arg[slot]);
            return false;
        }
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
                                        | dbgio::Print_Audio,
                                    backend_dispatcher, context.arena())) {
            return 1;
        }
        return 0;
    }

    address::IoUri output_uri(context.arena());
    if (!parse_output_uri(args, output_uri)) {
        return 1;
    }

    core::ScopedPtr<sndio::ISink> output_sink;
    if (!open_output_sink(backend_dispatcher, io_config, output_uri, output_sink)) {
        return 1;
    }

    io_config.sample_spec = output_sink->sample_spec();
    io_config.frame_length = output_sink->frame_length();

    pipeline::ReceiverSourceConfig receiver_config;
    if (!build_receiver_config(args, receiver_config, context, *output_sink)) {
        return 1;
    }

    core::ScopedPtr<sndio::ISource> backup_source;
    core::ScopedPtr<pipeline::TranscoderSource> backup_transcoder;

    if (args.backup_given) {
        address::IoUri backup_uri(context.arena());
        if (!parse_backup_uri(args, backup_uri)) {
            return 1;
        }

        if (!open_backup_source(backend_dispatcher, io_config, backup_uri,
                                backup_source)) {
            return 1;
        }

        if (!open_backup_transcoder(backup_transcoder, *backup_source, context,
                                    receiver_config)) {
            return 1;
        }
    }

    node::Receiver receiver(context, receiver_config);
    if (receiver.init_status() != status::StatusOK) {
        roc_log(LogError, "can't create receiver node: status=%s",
                status::code_to_str(receiver.init_status()));
        return 1;
    }

    if (!prepare_receiver(args, context, receiver)) {
        return 1;
    }

    const sndio::IoPump::Mode pump_mode =
        args.oneshot_flag ? sndio::IoPump::ModeOneshot : sndio::IoPump::ModePermanent;

    sndio::IoPump pump(context.frame_pool(), context.frame_buffer_pool(),
                       receiver.source(), backup_transcoder.get(), *output_sink,
                       io_config, pump_mode);
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
