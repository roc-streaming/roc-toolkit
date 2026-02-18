/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_dbgio/print_supported.h"
#include "roc_address/protocol_map.h"
#include "roc_audio/channel_defs.h"
#include "roc_audio/channel_tables.h"
#include "roc_audio/format.h"
#include "roc_audio/pcm_subformat.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/printer.h"
#include "roc_fec/codec_map.h"

namespace roc {
namespace dbgio {

namespace {

enum { LineSize = 70 };

const char* interface_to_option(address::Interface iface) {
    switch (iface) {
    case address::Iface_Invalid:
        break;
    case address::Iface_Aggregate:
        break;
    case address::Iface_AudioSource:
        return "--source";
    case address::Iface_AudioRepair:
        return "--repair";
    case address::Iface_AudioControl:
        return "--control";
    case address::Iface_Max:
        break;
    }
    return NULL;
}

void print_interface_protos(core::Printer& prn,
                            address::Interface iface,
                            const core::StringList& list) {
    const char* iface_option = interface_to_option(iface);
    if (iface_option == NULL) {
        return;
    }

    const char* str = list.front();

    while (str != NULL) {
        prn.writef("  %-12s ", iface_option);

        size_t size = 0;
        while (size < LineSize) {
            size += prn.writef(" %s%s%s", "", str, "://");

            str = list.nextof(str);
            if (!str) {
                break;
            }
        }

        prn.writef("\n");
    }
}

bool print_network_schemes(core::Printer& prn, core::IArena& arena) {
    core::Array<address::Interface> interface_array(arena);
    core::StringList list(arena);

    if (!address::ProtocolMap::instance().get_supported_interfaces(interface_array)) {
        roc_log(LogError, "can't retrieve interface array");
        return false;
    }

    for (size_t n_interface = 0; n_interface < interface_array.size(); n_interface++) {
        if (!address::ProtocolMap::instance().get_supported_protocols(
                interface_array[n_interface], list)) {
            roc_log(LogError, "can't retrieve protocols list");
            return false;
        }

        if (n_interface == 0) {
            prn.writef("Supported uri schemes for network endpoints:  [NET_URI]\n");
        }

        print_interface_protos(prn, interface_array[n_interface], list);
    }
    return true;
}

void print_string_list(core::Printer& prn,
                       const core::StringList& list,
                       const char* prefix,
                       const char* suffix) {
    const char* str = list.front();

    while (str != NULL) {
        prn.writef(" ");

        size_t size = 0;
        while (size < LineSize) {
            size += prn.writef(" %s%s%s", prefix, str, suffix);

            str = list.nextof(str);
            if (!str) {
                break;
            }
        }

        prn.writef("\n");
    }
}

bool print_io_schemes(sndio::BackendDispatcher& backend_dispatcher,
                      core::Printer& prn,
                      core::IArena& arena) {
    core::StringList list(arena);

    if (!backend_dispatcher.get_supported_schemes(list)) {
        roc_log(LogError, "can't retrieve driver list");
        return false;
    }

    prn.writef("Supported uri schemes for io endpoints:  [IO_URI]\n");
    prn.writef("  (--input, --output)\n");
    print_string_list(prn, list, "", "://");

    return true;
}

bool print_network_formats(core::Printer& prn, core::IArena& arena) {
    prn.writef("Supported formats for network packets:  [PKT_ENCODING]\n");
    prn.writef("  (--packet-encoding)\n");

    prn.writef(" ");

    for (int fmt = audio::Format_Invalid; fmt < audio::Format_Max; fmt++) {
        if (fmt == audio::Format_Invalid) {
            continue;
        }
        const audio::FormatTraits traits = audio::format_traits((audio::Format)fmt);
        if (traits.has_flags(audio::Format_SupportsNetwork)) {
            prn.writef(" %s", traits.name);
        }
    }

    prn.writef("\n");

    return true;
}

bool print_device_formats(core::Printer& prn, core::IArena& arena) {
    prn.writef("Supported formats for device io:  [IO_ENCODING]\n");
    prn.writef("  (--io-encoding)\n");

    prn.writef(" ");

    for (int fmt = audio::Format_Invalid; fmt < audio::Format_Max; fmt++) {
        if (fmt == audio::Format_Invalid) {
            continue;
        }
        const audio::FormatTraits traits = audio::format_traits((audio::Format)fmt);
        if (traits.has_flags(audio::Format_SupportsDevices)) {
            prn.writef(" %s", traits.name);
        }
    }

    prn.writef("\n");

    return true;
}

bool print_file_formats(sndio::BackendDispatcher& backend_dispatcher,
                        core::Printer& prn,
                        core::IArena& arena) {
    core::StringList list(arena);

    if (!backend_dispatcher.get_supported_formats(list)) {
        roc_log(LogError, "can't retrieve format list");
        return false;
    }

    list.sort(core::StringList::OrderNatural);

    prn.writef("Supported formats for file io:  [IO_ENCODING]\n");
    prn.writef("  (--io-encoding)\n");
    print_string_list(prn, list, "", "");

    return true;
}

bool print_pcm_subformats(core::Printer& prn, core::IArena& arena) {
    prn.writef("Supported pcm sub-formats:"
               "  [PKT_ENCODING, IO_ENCODING]\n");
    prn.writef("  (--packet-encoding, --io-encoding)\n");

    bool first = true;
    audio::PcmTraits prev_traits, curr_traits;

    for (int n = 0; n < audio::PcmSubformat_Max; n++) {
        const audio::PcmSubformat fmt = (audio::PcmSubformat)n;
        if (fmt == audio::PcmSubformat_Invalid) {
            continue;
        }

        curr_traits = pcm_subformat_traits(fmt);

        if (prev_traits.bit_depth != curr_traits.bit_depth
            || prev_traits.bit_width != curr_traits.bit_width) {
            if (curr_traits.bit_width % 8 == 0) {
                prn.writef("%s  %2d bit (%d byte)    ", first ? "" : "\n",
                           (int)curr_traits.bit_depth, (int)curr_traits.bit_width / 8);
            } else {
                prn.writef("%s  %d bit (%.2f byte) ", first ? "" : "\n",
                           (int)curr_traits.bit_depth,
                           (double)curr_traits.bit_width / 8.);
            }
            first = false;
        } else if (prev_traits.has_flags(audio::Pcm_IsSigned)
                   != curr_traits.has_flags(audio::Pcm_IsSigned)) {
            prn.writef("  ");
        }

        prev_traits = curr_traits;

        prn.writef(" %s", pcm_subformat_to_str(fmt));
    }

    prn.writef("\n");

    return true;
}

bool print_file_subformats(sndio::BackendDispatcher& backend_dispatcher,
                           core::Printer& prn,
                           core::IArena& arena) {
    core::StringList groups(arena);
    core::StringList subformats(arena);

    if (!backend_dispatcher.get_supported_subformat_groups(groups)) {
        return false;
    }

    bool first = true;

    for (const char* grp = groups.front(); grp != NULL; grp = groups.nextof(grp)) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        prn.writef("Supported %s sub-formats:"
                   "  [IO_ENCODING]\n",
                   grp);
        prn.writef("  (--io-encoding)\n");

        if (!backend_dispatcher.get_supported_subformats(grp, subformats)) {
            return false;
        }

        subformats.sort(core::StringList::OrderNatural);

        prn.writef(" ");

        for (const char* subfmt = subformats.front(); subfmt != NULL;
             subfmt = subformats.nextof(subfmt)) {
            prn.writef(" %s", subfmt);
        }

        prn.writef("\n");
    }

    return true;
}

bool print_channel_masks(core::Printer& prn, core::IArena& arena) {
    prn.writef("Supported channel masks:"
               "  [PKT_ENCODING, IO_ENCODING]\n");
    prn.writef("  (--packet-encoding, --io-encoding)\n");

    for (size_t i = 0; i < ROC_ARRAY_SIZE(audio::ChanMaskNames); i++) {
        const audio::ChannelMask ch_mask = audio::ChanMaskNames[i].mask;

        // TODO(gh-696): finish surround and enable all masks.
        if (ch_mask != audio::ChanMask_Surround_Mono
            && ch_mask != audio::ChanMask_Surround_Stereo) {
            continue;
        }

        prn.writef("  %-13s  (", audio::channel_mask_to_str(ch_mask));

        bool first = true;

        for (int ch = 0; ch < audio::ChanPos_Max; ch++) {
            if (ch_mask & (1 << ch)) {
                if (!first) {
                    prn.writef(" ");
                }
                first = false;
                prn.writef("%s", audio::channel_pos_to_str((audio::ChannelPosition)ch));
            }
        }

        prn.writef(")\n");
    }

    return true;
}

bool print_channel_names(core::Printer& prn, core::IArena& arena) {
    prn.writef("Supported surround channels:"
               "  [PKT_ENCODING, IO_ENCODING]\n");

    prn.writef("  front      FL FR FC\n");
    prn.writef("  side       SL SR\n");
    prn.writef("  back       BL BR BC\n");
    prn.writef("  top front  TFL TFR\n");
    prn.writef("  top mid    TML TMR\n");
    prn.writef("  top back   TBL TBR\n");
    prn.writef("  low freq   LFE\n");

    return true;
}

bool print_fec_schemes(core::Printer& prn, core::IArena& arena) {
    prn.writef("Supported fec encodings:  [FEC_ENCODING]\n");
    prn.writef("  (--fec-encoding)\n");

    const size_t n_schemes = fec::CodecMap::instance().num_schemes();

    if (n_schemes == 0) {
        prn.writef("  none");
    } else {
        prn.writef("  auto");

        for (size_t n = 0; n < n_schemes; n++) {
            prn.writef(
                " %s",
                packet::fec_scheme_to_str(fec::CodecMap::instance().nth_scheme(n)));
        }
    }

    prn.writef("\n");

    return true;
}

void print_section(core::Printer& prn, const char* section) {
    prn.writef("[[ %s ]]\n\n", section);
}

} // namespace

bool print_supported(unsigned flags,
                     sndio::BackendDispatcher& backend_dispatcher,
                     core::IArena& arena) {
    core::Printer prn;
    bool first = true;

    if (flags & Print_Netio) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        print_section(prn, "URI schemes");

        if (!print_network_schemes(prn, arena)) {
            return false;
        }
    }

    if (flags & Print_Sndio) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        if (!print_io_schemes(backend_dispatcher, prn, arena)) {
            return false;
        }
    }

    if (flags & Print_Netio) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        print_section(prn, "Formats");

        if (!print_network_formats(prn, arena)) {
            return false;
        }
    }

    if (flags & Print_Sndio) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        if (!print_device_formats(prn, arena)) {
            return false;
        }

        prn.writef("\n");

        if (!print_file_formats(backend_dispatcher, prn, arena)) {
            return false;
        }
    }

    if (flags & Print_Audio) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        print_section(prn, "Sub-formats");

        if (!print_pcm_subformats(prn, arena)) {
            return false;
        }
    }

    if (flags & Print_Sndio) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        if (!print_file_subformats(backend_dispatcher, prn, arena)) {
            return false;
        }
    }

    if (flags & Print_Audio) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        print_section(prn, "Channels");

        if (!print_channel_masks(prn, arena)) {
            return false;
        }
    }

    if (flags & Print_FEC) {
        if (first) {
            first = false;
        } else {
            prn.writef("\n");
        }

        print_section(prn, "FEC");

        if (!print_fec_schemes(prn, arena)) {
            return false;
        }
    }

    return true;
}

} // namespace dbgio
} // namespace roc
