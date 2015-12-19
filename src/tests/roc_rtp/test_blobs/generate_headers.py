#! /usr/bin/python2
from __future__ import print_function
import sys
import os.path
import re
import glob
import json

def print_comment(key, indent=1):
    print("%s/* %-16s */ " % (" " * indent * 2, key), end="")

def print_number(key, value):
    print_comment(key)
    print("%s," % value)

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

for in_json in glob.glob("*.json"):
    in_name = re.sub("\.json$", "", in_json)
    in_blob = in_name + ".blob"

    if not os.path.exists(in_blob):
        print("%s doesn't exist, skipping %s" % (in_blob, in_json), file=sys.stderr)
        continue

    out_h = in_name + ".h"
    print("writing %s" % out_h, file=sys.stderr)

    guard = 'ROC_RTP_TEST_BLOBS_' + out_h.upper().replace('.', '_') + '_'

    meta = json.loads(open(in_json).read())

    with open(in_blob) as blob:
        raw_data = blob.read()

    with open(out_h, 'w') as sys.stdout:
        print(("""
/*
 * THIS FILE IS AUTO-GENERATED USING `generate_headers.py'
 *
 * Input:
 *  - %s
 *  - %s
 */

#ifndef %s
#define %s

#include "test_rtp_packet.h"

namespace roc {
namespace test {

static RTP_PacketTest %s = {
        """ % (
            in_json,
            in_blob,
            guard,
            guard,
            in_name)).strip())

        print_blob("raw_data", raw_data)

        print()
        print_number("packet_size", len(raw_data))
        print_number("header_size", meta["header_size"])
        print_number("extension_size", meta["extension_size"])
        print_number("payload_size", meta["payload_size"])
        print_number("padding_size", meta["padding_size"])

        print()
        print_number("version", meta["version"])
        print_bool("padding", meta["padding"])
        print_bool("extension", meta["extension"])
        print_number("num_csrc", len(meta["csrc"]))
        print_number("pt", meta["pt"])
        print_bool("marker", meta["marker"])

        print()
        print_number("seqnum", meta["seqnum"])
        print_number("ts", meta["ts"])
        print_number("ssrc", meta["ssrc"])
        print_array("csrc", meta["csrc"])

        print()
        print_number("ext_type", meta["ext_type"])
        print_number("ext_data_size", len(meta["ext_data"]))
        print_blob("ext_data", meta["ext_data"])

        print()
        print_number("num_channels", meta["num_channels"])
        print_number("num_samples", meta["num_samples"])
        print_number("samplebits", meta["samplebits"])

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
} // namespace roc

#endif // %s
        """ % (guard)).strip())
