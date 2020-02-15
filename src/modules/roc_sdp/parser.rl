/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <iostream>
#include <string.h>

#include "roc_sdp/session_description.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/array.h"
#include "roc_core/heap_allocator.h"

namespace roc {
namespace sdp {

%%{
    machine parse_sdp;
    write data;
}%%

bool parse_sdp(const char* str, SessionDescription& result) {

    // for ragel
    const char* p = str;
    const char *pe = str + strlen(str);

    const char *eof = pe;
    int cs = 0;

    // for start_token
    const char* start_p = NULL;

    // parse result
    bool success = false;

    const char* start_p_origin_username = NULL;
    const char* end_p_origin_sess_id = NULL;
    const char* start_p_origin_nettype = NULL;
    const char* end_p_origin_addr = NULL;

    address::AddrFamily session_connection_addrtype = address::Family_Unknown;

    %%{

        action start_token {
            start_p = p;
        }

        action set_origin_unicast_address {
            if(!result.set_origin_unicast_address(start_p, p - start_p)) {
                roc_log(LogError, "parse origin unicast address: invalid address");
                result.clear();
                return false;
            }
        }

        action set_guid {
            if(!result.set_guid(start_p_origin_username, 
                                end_p_origin_sess_id,
                                start_p_origin_nettype,
                                end_p_origin_addr)) {
                roc_log(LogError, "parse origin guid: invalid origin field");
                result.clear();
                return false;
            }
        }

        action set_session_connection_address {
            if(!result.set_session_connection_address(
                    session_connection_addrtype,
                    start_p, 
                    p - start_p)) {
                        
                roc_log(LogError, "parse session connection address: invalid address");
                result.clear();
                return false;
            }
        }

        ##### USEFUL GRAMMAR #####
        # ABNF: 1*(VCHAR/%x80-FF) -> string of visible characters
        non_ws_string = [!-~]+;      

        # ABNF: %x21 / %x23-27 / %x2A-2B / %x2D-2E / %x30-39 / %x41-5A / %x5E-7E
        token_char = ([!#\$%&\'\*\+\-\.\^_\`\{\|\}~] | alnum);
        token = token_char+;

        ##### SDP ATTRIBUTES - NOT YET IMPLEMENTED #####
        # <payload type> <encoding name>/<clock rate> [/<encoding parameters>]
        # (dynamic payload ID: encoding name, sample rate, channel set)
        a_rtpmap = "a=rtpmap";

        a_recvonly = "a=recvonly"; # (session mode / direction)
        a_sendrecv = "a=sendrecv"; # (--//--)
        a_sendonly = "a=sendonly"; # (--//--)
        a_inactive = "a=inactive"; # (--//--)
        a_type = "a=type"; # (session type; defines default session mode if omitted)
        a_fmtp = "a=fmtp"; # (codec-specific parameters; we'll need it for Opus)
        a_fec_source_flow = "a=fec-source-flow"; # (FECFRAME; see RFC 6364)
        a_fec_repair_flow = "a=fec-repair-flow"; # (--//--)
        a_repair_window = "a=repair-window"; # (--//--)

        ##### SDP FIELDS #####
        version = digit+;

        # o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
        origin_username = non_ws_string >start_token %{ start_p_origin_username = start_p; };
        origin_sess_id = digit+ >start_token %{ end_p_origin_sess_id = p; };
        origin_nettype = "IN" >start_token %{ start_p_origin_nettype = start_p; };
        origin_addrtype = ("IP4" %{ result.set_origin_addrtype(address::Family_IPv4); } 
                            | "IP6" %{ result.set_origin_addrtype(address::Family_IPv6); }
                        );
        origin_unicast_address = non_ws_string >start_token 
            %{  end_p_origin_addr = p; } 
            %set_origin_unicast_address;

        # action todo: sess-id should be unique for this username/host
        origin = origin_username ' ' origin_sess_id ' '  digit+ ' ' 
            origin_nettype ' ' origin_addrtype ' ' origin_unicast_address %set_guid;

        
        # In session-level: c=<nettype> <addrtype> <connection-address>
        session_connection_nettype = "IN";
        session_connection_addrtype =  
            ( "IP4" %{ session_connection_addrtype = address::Family_IPv4; } 
            | "IP6" %{ session_connection_addrtype = address::Family_IPv6; } );

        session_connection_address = non_ws_string >start_token 
            %{  end_p_origin_addr = p; } 
            %set_session_connection_address;

        session_connection_data = session_connection_nettype ' ' 
            session_connection_addrtype ' ' session_connection_address;
    
        # m=<type> <port> <proto> <fmt> - NOT YET STORED
        # typically "audio", "video", "text", or "application"
        media_type = "audio"  
            | "video" 
            | "text" 
            | "application";
            
        # typically an RTP payload type for audio and video media
        media_fmt = token;
        # typically "RTP/AVP" or "udp"
        media_proto = token ("/" token)*;
        media_port = digit+;
        media_description = media_type ' ' media_port ' ' media_proto ' ' media_fmt;

        sdp_description = 'v='i version '\n'
        'o='i origin '\n'
        'c='i session_connection_data '\n'
        'm='i media_description;

        main := sdp_description
                %{ success = true; }
                ;

        write init;
        write exec;
    }%%

    if (!success) {
        return false;
    }

    return true;
}

} // namespace sdp
} // namespace roc
