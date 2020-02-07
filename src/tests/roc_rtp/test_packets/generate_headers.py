#! /usr/bin/python2
from __future__ import print_function
import sys
import os.path
import re
import glob
import json

def print_comment(key, indent=1):
    print("%s/* %-16s */ " % (" " * indent * 2, key), end="")

def print_uint(key, value):
    print_comment(key)
    print("%su," % value)

def print_bool(key, value):
    print_comment(key)
    print("%s," % ("1" if value else "0"))

def print_array(key, array, indent=1):
    if len(array) > 12:
        ret = "{\n"
        while array:
            ret += " " * (indent * 2 + 1)
            for n in range(12):
                if array:
                    ret += " %s," % array.pop(0)
            ret += "\n"
        ret += " " * (indent * 2)
        ret += "},"
    else:
        ret = "{ "
        ret += ", ".join(map(str, array))
        ret += " },"

    print_comment(key, indent)
    print(ret)

def print_blob(key, blob):
    print_array(key, ["0x%02x" % ord(b) for b in blob[:]])

if len(sys.argv) != 1:
    print("usage: generate_headers.py")
    exit(1)

os.chdir(os.path.dirname(os.path.abspath(__file__)))

for in_json in glob.glob("*.json"):
    in_name = re.sub("\.json$", "", in_json)
    in_blob = in_name + ".blob"

    if not os.path.exists(in_blob):
        print("%s doesn't exist, skipping %s" % (in_blob, in_json), file=sys.stderr)
        continue

    out_h = in_name + ".h"
    print("writing %s" % out_h, file=sys.stderr)

    guard = 'ROC_RTP_TEST_PACKETS_' + out_h.upper().replace('.', '_') + '_'

    meta = json.loads(open(in_json).read())

    with open(in_blob) as blob:
        raw_data = blob.read()

    with open(out_h, 'w') as sys.stdout:
        print(("""
/*
 * THIS FILE IS AUTO-GENERATED USING `generate_headers.py'
 *
 * Inputs:
 *  - %s
 *  - %s
 */

#ifndef %s
#define %s

#include "test_packets/packet_info.h"

namespace roc {
namespace rtp {
namespace test {

static PacketInfo %s = {
        """ % (
            in_json,
            in_blob,
            guard,
            guard,
            in_name)).strip())

        print_blob("raw_data", raw_data)

        print()
        print_uint("packet_size", len(raw_data))
        print_uint("header_size", meta["header_size"])
        print_uint("extension_size", meta["extension_size"])
        print_uint("payload_size", meta["payload_size"])
        print_uint("padding_size", meta["padding_size"])

        print()
        print_uint("version", meta["version"])
        print_bool("padding", meta["padding"])
        print_bool("extension", meta["extension"])
        print_uint("num_csrc", len(meta["csrc"]))
        print_uint("pt", meta["pt"])
        print_bool("marker", meta["marker"])

        print()
        print_uint("seqnum", meta["seqnum"])
        print_uint("ts", meta["ts"])
        print_uint("ssrc", meta["ssrc"])
        print_array("csrc", meta["csrc"])

        print()
        print_uint("ext_type", meta["ext_type"])
        print_uint("ext_data_size", len(meta["ext_data"]))
        print_blob("ext_data", meta["ext_data"])

        print()
        print_uint("num_channels", meta["num_channels"])
        print_uint("num_samples", meta["num_samples"])
        print_uint("samplebits", meta["samplebits"])
        print_uint("samplerate", meta["samplerate"])

        print()
        print_comment("samples")
        print("{")
        n_ch = 0
        for ch in meta["samples"]:
            print_array("channel #%s" % n_ch, ch, indent=2)
            n_ch += 1
        print("  },")

        print(("""
};

} // namespace test
} // namespace rtp
} // namespace roc

#endif // %s
        """ % (guard)).strip())
