#! /usr/bin/env python3

# This script generates downmixing tables for ChannelMapperMatrix.
# Its logic is complicated, but the result (C++ file with tables)
# is quite straightforward to read and verify.

# This approach makes C++ code simpler - it just needs to build
# a matrix from tables and apply it to stream.

from collections import OrderedDict
from fnmatch import fnmatch
from itertools import islice
from math import *
import builtins
import os
import os.path
import re
import sys

try:
    import jinja2
except ImportError:
    print('''
error: can't import python module "jinja2", install it using "pip3 install Jinja2"
'''.strip(), file=sys.stderr)
    exit(1)

CHANS = OrderedDict([
    ('FL', 'FrontLeft'),
    ('FLC', 'FrontLeftOfCenter'),
    ('FC', 'FrontCenter'),
    ('FRC', 'FrontRightOfCenter'),
    ('FR', 'FrontRight'),
    ('SL', 'SideLeft'),
    ('SR', 'SideRight'),
    ('BL', 'BackLeft'),
    ('BC', 'BackCenter'),
    ('BR', 'BackRight'),
    ('TFL', 'TopFrontLeft'),
    ('TFR', 'TopFrontRight'),
    ('TML', 'TopMidLeft'),
    ('TMR', 'TopMidRight'),
    ('TBL', 'TopBackLeft'),
    ('TBR', 'TopBackRight'),
    ('LFE', 'LowFrequency'),
])

ORDERS = OrderedDict([
    ('none', {
        'id':        'None',
        'text_name': 'none',
        'chans':     [],
        }),
    ('smpte', {
        'id':        'Smpte',
        'text_name': 'smpte',
        'chans':     'FL FR FC LFE BL BR FLC FRC BC SL SR TFL TFR TML TMR TBL TBR'.split(),
        }),
    ('alsa', {
        'id':        'Alsa',
        'text_name': 'alsa',
        'chans':     'FL FR BL BR FC LFE SL SR BC'.split(),
        }),
])

MASKS = OrderedDict([
    # 1.x
    ('1.0', {
        'id':        'Mono',
        'text_name': 'mono',
        'chans':     'FC'.split(),
        'map_to':    [],
     }),
    ('1.1', {
        'id':        '1_1',
        'chans':     'FC LFE'.split(),
        'map_to':    ['1.1*'],
     }),
    ('1.1-3c', {
        'id':        '1_1_3c',
        'chans':     'FLC FC FRC LFE'.split(),
        'map_to':    ['1.1*'],
     }),
    # 2.x
    ('2.0', {
        'id':        'Stereo',
        'text_name': 'stereo',
        'chans':     'FL FR'.split(),
        'map_to':    [],
     }),
    ('2.1', {
        'id':        '2_1',
        'text_name': 'surround2.1',
        'chans':     'FL FR LFE'.split(),
        'map_to':    ['1.1*', '2.1*'],
     }),
    # 3.x
    ('3.0', {
        'id':        '3_0',
        'text_name': 'surround3.0',
        'chans':     'FL FC FR'.split(),
        'map_to':    [],
     }),
    ('3.1', {
        'id':        '3_1',
        'text_name': 'surround3.1',
        'chans':     'FL FC FR LFE'.split(),
        'map_to':    ['1.1*', '2.1*', '3.1*'],
     }),
    ('3.1-3c', {
        'id':        '3_1_3c',
        'chans':     'FL FLC FC FRC FR LFE'.split(),
        'map_to':    ['1.1*', '2.1*', '3.1*'],
     }),
    # 4.x
    ('4.0', {
        'id':        '4_0',
        'text_name': 'surround4.0',
        'chans':     'FL FR BL BR'.split(),
        'map_to':    [],
     }),
    ('4.1', {
        'id':        '4_1',
        'text_name': 'surround4.1',
        'chans':     'FL FR BL BR LFE'.split(),
        'map_to':    ['2.1*', '3.1*', '4.1*'],
     }),
    # 5.x
    ('5.0', {
        'id':        '5_0',
        'text_name': 'surround5.0',
        'chans':     'FL FC FR BL BR'.split(),
        'map_to':    [],
     }),
    ('5.1', {
        'id':        '5_1',
        'text_name': 'surround5.1',
        'chans':     'FL FC FR BL BR LFE'.split(),
        'map_to':    ['3.1*', '4.1*', '5.1*'],
     }),
    ('5.1-3c', {
        'id':        '5_1_3c',
        'chans':     'FL FLC FC FRC FR BL BR LFE'.split(),
        'map_to':    ['3.1*', '4.1*', '5.1*'],
     }),
    ('5.1.2', {
        'id':        '5_1_2',
        'text_name': 'surround5.1.2',
        'chans':     'FL FC FR BL BR TML TMR LFE'.split(),
        'map_to':    ['3.1*', '4.1*', '5.1*'],
     }),
    ('5.1.2-3c', {
        'id':        '5_1_2_3c',
        'chans':     'FL FLC FC FRC FR BL BR TML TMR LFE'.split(),
        'map_to':    ['3.1*', '4.1*', '5.1*'],
     }),
    ('5.1.4', {
        'id':        '5_1_4',
        'text_name': 'surround5.1.4',
        'chans':     'FL FC FR BL BR TFL TFR TBL TBR LFE'.split(),
        'map_to':    ['3.1*', '4.1*', '5.1*'],
     }),
    ('5.1.4-3c', {
        'id':        '5_1_4_3c',
        'chans':     'FL FLC FC FRC FR BL BR TFL TFR TBL TBR LFE'.split(),
        'map_to':    ['3.1*', '4.1*', '5.1*'],
     }),
    # 6.x
    ('6.0', {
        'id':        '6_0',
        'text_name': 'surround6.0',
        'chans':     'FL FC FR BL BC BR'.split(),
        'map_to':    [],
     }),
    ('6.1', {
        'id':        '6_1',
        'text_name': 'surround6.1',
        'chans':     'FL FC FR BL BC BR LFE'.split(),
        'map_to':    ['4.1*', '5.1*', '6.1*'],
     }),
    ('6.1-3c', {
        'id':        '6_1_3c',
        'chans':     'FL FLC FC FRC FR BL BC BR LFE'.split(),
        'map_to':    ['4.1*', '5.1*', '6.1*'],
     }),
    # 7.x
    ('7.0', {
        'id':        '7_0',
        'text_name': 'surround7.0',
        'chans':     'FL FC FR SL SR BL BR'.split(),
        'map_to':    [],
     }),
    ('7.1', {
        'id':        '7_1',
        'text_name': 'surround7.1',
        'chans':     'FL FC FR SL SR BL BR LFE'.split(),
        'map_to':    ['5.1*', '6.1*', '7.1*'],
     }),
    ('7.1-3c', {
        'id':        '7_1_3c',
        'chans':     'FL FLC FC FRC FR SL SR BL BR LFE'.split(),
        'map_to':    ['5.1*', '6.1*', '7.1*'],
     }),
    ('7.1.2', {
        'id':        '7_1_2',
        'text_name': 'surround7.1.2',
        'chans':     'FL FC FR SL SR BL BR TML TMR LFE'.split(),
        'map_to':    ['5.1*', '6.1*', '7.1*'],
     }),
    ('7.1.2-3c', {
        'id':        '7_1_2_3c',
        'chans':     'FL FLC FC FRC FR SL SR BL BR TML TMR LFE'.split(),
        'map_to':    ['5.1*', '6.1*', '7.1*'],
     }),
    ('7.1.4', {
        'id':        '7_1_4',
        'text_name': 'surround7.1.4',
        'chans':     'FL FC FR SL SR BL BR TFL TFR TBL TBR LFE'.split(),
        'map_to':    ['5.1*', '6.1*', '7.1*'],
     }),
    ('7.1.4-3c', {
        'id':        '7_1_4_3c',
        'chans':     'FL FLC FC FRC FR SL SR BL BR TFL TFR TBL TBR LFE'.split(),
        'map_to':    ['5.1*', '6.1*', '7.1*'],
     }),
])

# List all channel names.
def chan_pos_names():
    ret = []
    for ch_name, ch_id in CHANS.items():
        ret += [(ch_id, ch_name)]
    return ret

# List all channel masks that have text names.
def chan_mask_names():
    ret = []
    for mask in MASKS.values():
        if mask.get('text_name', None):
            ret += [(mask['id'], mask['text_name'])]
    return ret

# List all channel orders.
def chan_orders():
    ret = []
    for order in ORDERS.values():
        ret += [(order['id'], order['text_name'], order['chans'])]
    return ret

# List all channel mask pairs for which we should generate mappings.
def chan_mappings():
    def want_mapping(src_name, src_mask, dst_name, dst_mask):
        # If map_to patterns of src don't cover dst, skip.
        # This limits how much lower levels to cover, e.g. there are direct
        # mappings of 7.x to 6.x and 5.x, but not 3.x.
        for pat in src_mask['map_to']:
            if fnmatch(dst_name, pat):
                break
        else:
            return False

        if '3c' not in src_name and '3c' in dst_name:
            # Non-3c is mapped only to non-3c.
            # (e.g. 5.1.2 is mapped to 3.1.2, but not 3.1.2-3c)
            return False

        if '3c' in src_name and '3c' not in dst_name and not src_name.startswith(dst_name):
            # 3c is mapped only to:
            #   - any 3c
            #   - non-3c of same level
            # (e.g. 5.1.2-3c is mapped to 3.1.2-3c and 5.1.2, but not 3.1.2)
            return False

        return True

    ret = []
    for src_idx, (src_name, src_mask) in islice(enumerate(MASKS.items()), 1, None):
        src_start = True
        for dst_idx, (dst_name, dst_mask) in islice(enumerate(MASKS.items()), 0, src_idx):
            # Here we skip some of the redundant mappings.
            # Theoretically we could generate mappings for all mask combinations,
            # but there would be just too much tables.
            # When there is no table for direct conversion, channel mapper will
            # use cascade downmixing/upmixing and combine multiple tables.
            if not want_mapping(src_name, src_mask, dst_name, dst_mask):
                continue

            ret += [(src_start, src_name, src_mask, dst_name, dst_mask)]
            src_start = False

    return ret

# For given dst chan, returns list of src chans with coefficients.
# E.g. for 'FC' it can return [('FL', 1.000), ('FR', 1.000)], depending
# on src mask and dst mask.
#
# This function was designed to produce downmixing tables equivalent to those
# defined in ITU and AC-3 standards, referenced in channel_tables.h.
# It, presumably, replicates heuristics used to define the tables,
# and allows to extend their usage to combinations not covered in documents.
#
# I don't believe that all these formulas are correct, though generated tables
# look sensible. Also, cascade down/up-mixing seems to give the same result
# as the direct mapping, as expected. But this definitely needs more eyeballs.
def map_chan(dst_chan, dst_mask, src_mask):
    def is_f(chan):
        return chan.startswith('F')
    def is_s(chan):
        return chan.startswith('S')
    def is_b(chan):
        return chan.startswith('B')
    def is_t(chan):
        return chan.startswith('T')
    def is_l(chan):
        return chan.endswith('L') or chan.endswith('LC')
    def is_r(chan):
        return chan.endswith('R') or chan.endswith('RC')

    def to_l(chan):
        chan = re.sub('R$', 'L', chan)
        chan = re.sub('RC$', 'LC', chan)
        return chan
    def to_r(chan):
        chan = re.sub('L$', 'R', chan)
        chan = re.sub('LC$', 'RC', chan)
        return chan

    def r_from_l():
        for chan, coeff in map_chan(to_l(dst_chan), dst_mask, src_mask):
            yield (to_r(chan), coeff)

    def in_src(ch):
        return ch in src_mask
    def in_dst(ch):
        return ch in dst_mask

    _1_000 = 1
    _0_707 = 1 / sqrt(2)
    _0_500 = 1 / 2
    _0_354 = 1 / (2 * sqrt(2))

    match dst_chan:
        case 'FL':
            # mix front
            if in_src('FL'):
                yield ('FL', _1_000)
            if in_src('FLC') and not in_dst('FLC'):
                yield ('FLC', _0_707)
            if in_src('FC') and not in_dst('FC'):
                yield ('FC', _0_707)
            # mix surround
            if not in_dst('SL') and not in_dst('BL'):
                # SL and BL
                if in_src('SL') and in_src('BL'):
                    yield ('SL', _0_500)
                    yield ('BL', _0_500)
                elif in_src('SL'):
                    yield ('SL', _0_707)
                elif in_src('BL'):
                    yield ('BL', _0_707)
                # BC
                if in_src('BC') and not in_dst('BC'):
                    yield ('BC', _0_500)
            # mix top
            if not in_dst('TFL') and not in_dst('TML') and not in_dst('TBL'):
                if in_src('TFL'):
                    yield ('TFL', _0_707)
                if in_src('TML') and not in_dst('SL'):
                    if in_dst('BL'):
                        yield ('TML', _0_500)
                    else:
                        yield ('TML', _0_707)
                if in_src('TBL') and not in_dst('BL'):
                    yield ('TBL', _0_500)
        case 'FR':
            # symmetrical to FL
            yield from r_from_l()
        case 'FLC':
            # mix front
            if in_src('FLC'):
                yield ('FLC', _1_000)
            else:
                if in_src('FL'):
                    yield ('FL', _0_707)
                if in_src('FC'):
                    yield ('FC', _0_500)
            # mix surround and top as we would mix into FL
            for chan, coeff in map_chan('FL', dst_mask, src_mask):
                if is_l(chan) and (is_s(chan) or is_b(chan) or is_t(chan)):
                    yield (chan, coeff)
        case 'FRC':
            # symmetrical to FLC
            yield from r_from_l()
        case 'FC':
            # mix front
            if in_src('FL') and (not in_dst('FL') or not in_src('FC') or
                                 (in_src('BC') and not in_dst('BL'))):
                yield ('FL', _0_707)
            if in_src('FLC') and not in_dst('FLC'):
                yield ('FLC', _0_707)
            if in_src('FC'):
                yield ('FC', _1_000)
            if in_src('FRC') and not in_dst('FRC'):
                yield ('FRC', _0_707)
            if in_src('FR') and (not in_dst('FR') or not in_src('FC') or
                                 (in_src('BC') and not in_dst('BR'))):
                yield ('FR', _0_707)
            # mix surround
            if in_src('SL') and not in_dst('SL') and not in_dst('BL') and (
                    not in_dst('FL') or not in_src('FC')):
                yield ('SL', _0_500)
            if in_src('SR') and not in_dst('SR') and not in_dst('BR') and (
                    not in_dst('FR') or not in_src('FC')):
                yield ('SR', _0_500)
            if in_src('BL') and not in_dst('BL') and (
                    not in_dst('FL') or not in_src('FC') or in_src('BC')):
                yield ('BL', _0_500)
            if in_src('BC') and not in_dst('BC') and (
                    not in_dst('BL') and not in_dst('BR') and
                    not in_dst('FL') and not in_dst('FR')):
                yield ('BC', _0_707)
            if in_src('BR') and not in_dst('BR') and (
                    not in_dst('FR') or not in_src('FC') or in_src('BC')):
                yield ('BR', _0_500)
            # mix top
            if in_src('TFL') and not in_dst('TFL') and not in_dst('FL'):
                yield ('TFL', _0_500)
            if in_src('TFR') and not in_dst('TFR') and not in_dst('FR'):
                yield ('TFR', _0_500)
            if in_src('TML') and not in_dst('TML') and not in_dst('FL'):
                yield ('TML', _0_500)
            if in_src('TMR') and not in_dst('TMR') and not in_dst('FR'):
                yield ('TMR', _0_500)
            if in_src('TBL') and not in_dst('TBL') and not in_dst('FL'):
                yield ('TBL', _0_354)
            if in_src('TBR') and not in_dst('TBR') and not in_dst('FR'):
                yield ('TBR', _0_354)
        case 'SL':
            # mix surround
            if in_src('SL'):
                yield ('SL', _1_000)
            # mix top
            if in_src('TML') and not in_dst('TML'):
                yield ('TML', _0_707)
        case 'SR':
            # symmetrical to SL
            yield from r_from_l()
        case 'BL':
            # mix surround
            if in_src('SL') and not in_dst('SL'):
                yield ('SL', _1_000)
            if in_src('BL'):
                yield ('BL', _1_000)
            if in_src('BC') and not in_dst('BC'):
                yield ('BC', _0_707)
            # mix top
            if not in_dst('TFL') and not in_dst('TML') and not in_dst('TBL'):
                if in_src('TML') and not in_dst('SL'):
                    if in_dst('FL'):
                        yield ('TML', _0_500)
                    else:
                        yield ('TML', _0_707)
                if in_src('TBL') and not in_dst('TBL'):
                    yield ('TBL', _0_707)
        case 'BR':
            # symmetrical to BL
            yield from r_from_l()
        case 'BC':
            # mix surround
            if in_src('BL') and (not in_dst('BL') or not in_src('BC')):
                yield ('BL', _1_000)
            if in_src('BC'):
                yield ('BC', _1_000)
            if in_src('BR') and (not in_dst('BR') or not in_src('BC')):
                yield ('BR', _1_000)
            if in_src('SL') and not in_dst('SL'):
                yield ('SL', _1_000)
            if in_src('SR') and not in_dst('SR'):
                yield ('SR', _1_000)
            # mix top
            if in_src('TML') and not in_dst('TML') and not in_dst('TBL'):
                yield ('TML', _0_707)
            if in_src('TMR') and not in_dst('TMR') and not in_dst('TBR'):
                yield ('TMR', _0_707)
            if in_src('TBL') and not in_dst('TBL'):
                yield ('TBL', _0_707)
            if in_src('TBR') and not in_dst('TBR'):
                yield ('TBR', _0_707)
        case 'TML':
            # mix top
            if in_src('TML'):
                yield ('TML', _1_000)
            if in_src('TFL') and not in_dst('TFL'):
                yield ('TFL', _0_707)
            if in_src('TBL') and not in_dst('TBL'):
                yield ('TBL', _0_707)
            # mix front & back
            if not in_src('TML') and not in_src('TFL') and not in_src('TBL'):
                if in_src('FL'):
                    yield ('FL', _1_000)
                if in_src('BL'):
                    yield ('BL', _1_000)
                if in_src('BC'):
                    yield ('BC', _0_707)
        case 'TMR':
            # symmetrical to TML
            yield from r_from_l()
        case 'TFL':
            # mix top
            if in_src('TFL'):
                yield ('TFL', _1_000)
            if in_src('TML') and not in_dst('TML'):
                yield ('TML', _0_707)
                if in_src('TBL') and not in_dst('TBL'):
                    yield ('TBL', _0_500)
            elif in_src('TBL') and not in_dst('TBL'):
                yield ('TBL', _0_707)
            # mix front
            if not in_src('TML') and not in_src('TFL') and not in_src('TBL'):
                if in_src('FL'):
                    yield ('FL', _1_000)
        case 'TFR':
            # symmetrical to TFR
            yield from r_from_l()
        case 'TBL':
            # mix top
            if in_src('TBL'):
                yield ('TBL', _1_000)
            if in_src('TML') and not in_dst('TML'):
                yield ('TML', _0_707)
                if in_src('TFL') and not in_dst('TFL'):
                    yield ('TFL', _0_500)
            elif in_src('TFL') and not in_dst('TFL'):
                yield ('TBL', _0_707)
            # mix back
            if not in_src('TML') and not in_src('TFL') and not in_src('TBL'):
                if in_src('BL'):
                    yield ('BL', _1_000)
                if in_src('BC'):
                    yield ('BC', _0_707)
        case 'TBR':
            # symmetrical to TBR
            yield from r_from_l()
        case 'LFE':
            # LFE is just copied
            if in_src('LFE'):
                yield ('LFE', _1_000)
        case _:
            raise NotImplementedError(dst_chan)

env = jinja2.Environment(
    trim_blocks=True,
    lstrip_blocks=True,
    undefined = jinja2.StrictUndefined)

template = env.from_string('''
/*
 * THIS FILE IS AUTO-GENERATED USING `channel_tables_gen.py'. DO NOT EDIT!
 */

#include "roc_audio/channel_tables.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

// Table of channel position names.
const ChannelPositionName ChanPositionNames[{{ len(chan_pos_names()) }}] = {
{% for chan_id, chan_name in chan_pos_names() %}
    { "{{ chan_name }}", ChanPos_{{ chan_id }} },
{% endfor %}
};

// Table of channel mask names.
const ChannelMaskName ChanMaskNames[{{ len(chan_mask_names()) }}] = {
{% for mask_id, mask_name in chan_mask_names() %}
    { "{{ mask_name }}", ChanMask_Surround_{{ mask_id }} },
{% endfor %}
};

// Table of channel orders.
const ChannelOrderTable ChanOrderTables[{{ len(chan_orders()) }}] = {
{% for order_id, order_name, order_chans in chan_orders() %}
    {
        "{{ order_name }}",
        ChanOrder_{{ order_id }},
        {
{% for ch in order_chans %}
            ChanPos_{{ CHANS[ch] }},
{% endfor %}
            ChanPos_Max,
        },
    },
{% endfor %}
};

// Table of channel mappings.
const ChannelMapTable ChanMapTables[{{ len(chan_mappings()) }}] = {
{% for src_start, src_name, src_mask, dst_name, dst_mask in chan_mappings() %}
{% if src_start %}
    // {{ src_name }}<>...
{% endif %}
    {
        "{{ src_name }}<>{{ dst_name }}",
        ChanMask_Surround_{{ src_mask.id }},
        ChanMask_Surround_{{ dst_mask.id }},
        {
{% for dst_chan in dst_mask.chans %}
            // {{ dst_chan }}
{% for src_chan, coeff in map_chan(dst_chan, dst_mask.chans, src_mask.chans) %}
            { ChanPos_{{ CHANS[dst_chan] }}, ChanPos_{{ CHANS[src_chan] }}, {{ "%.7f"|format(coeff) }}f },
{% endfor %}
{% endfor %}
        },
    },
{% endfor %}
};

} // namespace audio
} // namespace roc
'''.strip())

text = template.render(
    **dict(list(globals().items()) + list(builtins.__dict__.items())),
    )

os.chdir(os.path.dirname(os.path.abspath(__file__)))

with open('channel_tables.cpp', 'w') as fp:
    print(text, file=fp)
