#! /usr/bin/env python3
from collections import OrderedDict
from itertools import islice
import builtins
import math
import os
import os.path
import sys

try:
    import jinja2
except ImportError:
    print('''
error: can't import python module "jinja2", install it using "pip3 install Jinja2"
'''.strip(), file=sys.stderr)
    exit(1)

def skip_all(mask):
    return True

def skip_all_except(*masks):
    def fn(mask):
        return mask not in masks
    return fn

CHANS = OrderedDict([
    ('FL', 'FrontLeft'),
    ('FC', 'FrontCenter'),
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
    ('None',
     ''.split()),
    ('Smpte',
     'FL FR FC LFE BL BR BC SL SR TFL TFR TML TMR TBL TBR'.split()),
    ('Alsa',
     'FL FR BL BR FC LFE SL SR BC'.split()),
])

MASKS = OrderedDict([
    ('1.0', {
        'id':        'Mono',
        'alias':     'mono',
        'chans':     'FC'.split(),
     }),
    ('2.0', {
        'id':        'Stereo',
        'alias':     'stereo',
        'chans':     'FL FR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('2.1', {
        'id':        '2_1',
        'alias':     'surround2.1',
        'chans':     'FL FR LFE'.split(),
     }),
    ('3.0', {
        'id':        '3_0',
        'alias':     'surround3.0',
        'chans':     'FL FC FR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('3.1', {
        'id':        '3_1',
        'alias':     'surround3.1',
        'chans':     'FL FC FR LFE'.split(),
     }),
    ('4.0', {
        'id':        '4_0',
        'alias':     'surround4.0',
        'chans':     'FL FR BL BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('4.1', {
        'id':        '4_1',
        'alias':     'surround4.1',
        'chans':     'FL FR BL BR LFE'.split(),
     }),
    ('5.0', {
        'id':        '5_0',
        'alias':     'surround5.0',
        'chans':     'FL FC FR BL BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('5.1', {
        'id':        '5_1',
        'alias':     'surround5.1',
        'chans':     'FL FC FR BL BR LFE'.split(),
        'skip_from':  skip_all_except('5.1.2', '5.1.4'),
        'skip_to':    skip_all,
     }),
    ('5.1.2', {
        'id':        '5_1_2',
        'alias':     'surround5.1.2',
        'chans':     'FL FC FR BL BR TML TMR LFE'.split(),
     }),
    ('5.1.4', {
        'id':        '5_1_4',
        'alias':     'surround5.1.4',
        'chans':     'FL FC FR BL BR TFL TFR TBL TBR LFE'.split(),
     }),
    ('6.0', {
        'id':        '6_0',
        'alias':     'surround6.0',
        'chans':     'FL FC FR BL BC BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('6.1', {
        'id':        '6_1',
        'alias':     'surround6.1',
        'chans':     'FL FC FR BL BC BR LFE'.split(),
     }),
    ('7.0', {
        'id':        '7_0',
        'alias':     'surround7.0',
        'chans':     'FL FC FR SL SR BL BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('7.1', {
        'id':        '7_1',
        'alias':     'surround7.1',
        'chans':     'FL FC FR SL SR BL BR LFE'.split(),
        'skip_from':  skip_all_except('7.1.2', '7.1.4'),
        'skip_to':    skip_all,
     }),
    ('7.1.2', {
        'id':        '7_1_2',
        'alias':     'surround7.1.2',
        'chans':     'FL FC FR SL SR BL BR TML TMR LFE'.split(),
     }),
    ('7.1.4', {
        'id':        '7_1_4',
        'alias':     'surround7.1.4',
        'chans':     'FL FC FR SL SR BL BR TFL TFR TBL TBR LFE'.split(),
     }),
])

# Returns True if we should skip mapping src_mask => dst_mask.
def skip_mapping(src_name, src_mask, dst_name, dst_mask):
    if 'skip_from' in dst_mask:
        if dst_mask['skip_from'](src_name):
            return True
    if 'skip_to' in src_mask:
        if src_mask['skip_to'](dst_name):
            return True
    return False

# List all channel mask pairs for which we should generate mappings.
# Returns list like [(src_idx, src_name, src_mask, dst_idx, dst_name, dst_mask), ...]
def all_mappings():
    ret = []
    for src_idx, (src_name, src_mask) in islice(enumerate(MASKS.items()), 1, None):
        for dst_idx, (dst_name, dst_mask) in islice(enumerate(MASKS.items()), 0, src_idx):
            if not skip_mapping(src_name, src_mask, dst_name, dst_mask):
                ret += [(src_idx, src_name, src_mask,
                    dst_idx, dst_name, dst_mask)]
    return ret

# For given dst chan, returns list of src chans with coefficients.
# E.g. for 'FC' it can return [('FL', 1.000), ('FR', 1.000)], depending
# on src mask and dst mask.
# This function was designed to produce downmixing tables equivalent to those
# defined in ITU and AC-3 standards, referenced in channel_tables.h.
# It, presumably, replicates heuristics used to define the tables,
# and allows to extend their usage to combinations not covered in documents.
def map_chan(dst_chan, dst_mask, src_mask):
    match dst_chan:
        case 'FL':
            if 'FL' in src_mask:
                yield ('FL', 1.000)
            if 'FC' in src_mask and 'FC' not in dst_mask:
                yield ('FC', 0.707)
            if 'SL' in src_mask and ('SL' not in dst_mask and 'BL' not in dst_mask):
                yield ('SL', 0.707)
            if 'BL' in src_mask and 'BL' not in dst_mask:
                yield ('BL', 0.707)
            if 'BC' in src_mask and ('BC' not in dst_mask and 'BL' not in dst_mask):
                yield ('BC', 0.500)
            if 'TFL' in src_mask and 'TFL' not in dst_mask:
                yield ('TFL', 0.707)
            if 'TML' in src_mask and ('TML' not in dst_mask and 'TFL' not in dst_mask and
                                      'SL' not in dst_mask):
                yield ('TML', 0.707)
            if 'TBL' in src_mask and ('TBL' not in dst_mask and 'BL' not in dst_mask):
                yield ('TBL', 0.500)
        case 'FC':
            if 'FL' in src_mask and ('FL' not in dst_mask or 'FC' not in src_mask or
                                     ('BC' in src_mask and 'BL' not in dst_mask)):
                yield ('FL', 0.707)
            if 'FC' in src_mask:
                yield ('FC', 1.000)
            if 'FR' in src_mask and ('FR' not in dst_mask or 'FC' not in src_mask or
                                     ('BC' in src_mask and 'BR' not in dst_mask)):
                yield ('FR', 0.707)
            if 'SL' in src_mask and ('SL' not in dst_mask and (
                'FL' not in dst_mask or 'FC' not in src_mask)):
                yield ('SL', 0.500)
            if 'SR' in src_mask and ('SR' not in dst_mask and (
                'FR' not in dst_mask or 'FC' not in src_mask)):
                yield ('SR', 0.500)
            if 'BL' in src_mask and ('BL' not in dst_mask and (
                'FL' not in dst_mask or 'FC' not in src_mask) or
                                     ('BC' in src_mask and 'BL' not in dst_mask)):
                yield ('BL', 0.500)
            if 'BC' in src_mask and ('BC' not in dst_mask and
                    ('BL' not in dst_mask and 'BR' not in dst_mask and
                     'FL' not in dst_mask and 'FR' not in dst_mask)):
                yield ('BC', 0.707)
            if 'BR' in src_mask and ('BR' not in dst_mask and (
                'FR' not in dst_mask or 'FC' not in src_mask) or
                                     ('BC' in src_mask and 'BR' not in dst_mask)):
                yield ('BR', 0.500)
            if 'TFL' in src_mask and ('TFL' not in dst_mask and 'FL' not in dst_mask):
                yield ('TFL', 0.500)
            if 'TFR' in src_mask and ('TFR' not in dst_mask and 'FR' not in dst_mask):
                yield ('TFR', 0.500)
            if 'TML' in src_mask and ('TML' not in dst_mask and 'FL' not in dst_mask):
                yield ('TML', 0.500)
            if 'TMR' in src_mask and ('TMR' not in dst_mask and 'FR' not in dst_mask):
                yield ('TMR', 0.500)
            if 'TBL' in src_mask and ('TBL' not in dst_mask and 'FL' not in dst_mask):
                yield ('TBL', 0.354)
            if 'TBR' in src_mask and ('TBR' not in dst_mask and 'FR' not in dst_mask):
                yield ('TBR', 0.354)
        case 'FR':
            if 'FR' in src_mask:
                yield ('FR', 1.000)
            if 'FC' in src_mask and 'FC' not in dst_mask:
                yield ('FC', 0.707)
            if 'SR' in src_mask and ('SR' not in dst_mask and 'BR' not in dst_mask):
                yield ('SR', 0.707)
            if 'BR' in src_mask and 'BR' not in dst_mask:
                yield ('BR', 0.707)
            if 'BC' in src_mask and ('BC' not in dst_mask and 'BR' not in dst_mask):
                yield ('BC', 0.500)
            if 'TFR' in src_mask and 'TFR' not in dst_mask:
                yield ('TFR', 0.707)
            if 'TMR' in src_mask and ('TMR' not in dst_mask and 'TFR' not in dst_mask and
                                      'SR' not in dst_mask):
                yield ('TMR', 0.707)
            if 'TBR' in src_mask and ('TBR' not in dst_mask and 'BR' not in dst_mask):
                yield ('TBR', 0.500)
        case 'SL':
            if 'SL' in src_mask:
                yield ('SL', 1.000)
            if 'TML' in src_mask and 'TML' not in dst_mask:
                yield ('TML', 0.707)
        case 'SR':
            if 'SR' in src_mask:
                yield ('SR', 1.000)
            if 'TMR' in src_mask and 'TMR' not in dst_mask:
                yield ('TMR', 0.707)
        case 'BL':
            if 'SL' in src_mask and 'SL' not in dst_mask:
                yield ('SL', 1.000)
            if 'BL' in src_mask:
                yield ('BL', 1.000)
            if 'BC' in src_mask and 'BC' not in dst_mask:
                yield ('BC', 0.707)
            if 'TML' in src_mask and ('TML' not in dst_mask and 'TFL' not in dst_mask and
                                      'SL' not in dst_mask):
                yield ('TML', 0.707)
            if 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
        case 'BC':
            if 'SL' in src_mask and 'SL' not in dst_mask:
                yield ('SL', 1.000)
            if 'SR' in src_mask and 'SR' not in dst_mask:
                yield ('SR', 1.000)
            if 'BL' in src_mask and ('BL' not in dst_mask or 'BC' not in src_mask):
                yield ('BL', 1.000)
            if 'BR' in src_mask and ('BR' not in dst_mask or 'BC' not in src_mask):
                yield ('BR', 1.000)
            if 'TML' in src_mask and ('TML' not in dst_mask and 'TBL' not in dst_mask):
                yield ('TML', 0.707)
            if 'TMR' in src_mask and ('TMR' not in dst_mask and 'TBR' not in dst_mask):
                yield ('TMR', 0.707)
            if 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
            if 'TBR' in src_mask and 'TBR' not in dst_mask:
                yield ('TBR', 0.707)
        case 'BR':
            if 'SR' in src_mask and 'SR' not in dst_mask:
                yield ('SR', 1.000)
            if 'BR' in src_mask:
                yield ('BR', 1.000)
            if 'BC' in src_mask and 'BC' not in dst_mask:
                yield ('BC', 0.707)
            if 'TMR' in src_mask and ('TMR' not in dst_mask and 'TFR' not in dst_mask and
                                      'SR' not in dst_mask):
                yield ('TMR', 0.707)
            if 'TBR' in src_mask and 'TBR' not in dst_mask:
                yield ('TBR', 0.707)
        case 'TML':
            if 'TML' in src_mask:
                yield ('TML', 1.000)
            if 'TFL' in src_mask and 'TFL' not in dst_mask:
                yield ('TFL', 0.707)
            if 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
            if 'TML' not in src_mask and 'TFL' not in src_mask and 'TBL' not in src_mask:
                if 'FL' in src_mask:
                    yield ('FL', 1.000)
                if 'BL' in src_mask:
                    yield ('BL', 1.000)
                if 'BC' in src_mask:
                    yield ('BC', 0.707)
        case 'TMR':
            if 'TMR' in src_mask:
                yield ('TMR', 1.000)
            if 'TFR' in src_mask and 'TFR' not in dst_mask:
                yield ('TFR', 0.707)
            if 'TBR' in src_mask and 'TBR' not in dst_mask:
                yield ('TBR', 0.707)
            if 'TMR' not in src_mask and 'TFR' not in src_mask and 'TBR' not in src_mask:
                if 'FR' in src_mask:
                    yield ('FR', 1.000)
                if 'BR' in src_mask:
                    yield ('BR', 1.000)
                if 'BC' in src_mask:
                    yield ('BC', 0.707)
        case 'TFL':
            if 'TFL' in src_mask:
                yield ('TFL', 1.000)
            if 'TML' in src_mask and 'TML' not in dst_mask:
                yield ('TML', 0.707)
                if 'TBL' in src_mask and 'TBL' not in dst_mask:
                    yield ('TBL', 0.500)
            elif 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
            if 'TML' not in src_mask and 'TFL' not in src_mask and 'TBL' not in src_mask:
                if 'FL' in src_mask:
                    yield ('FL', 1.000)
        case 'TFR':
            if 'TFR' in src_mask:
                yield ('TFR', 1.000)
            if 'TMR' in src_mask and 'TMR' not in dst_mask:
                yield ('TMR', 0.707)
                if 'TBR' in src_mask and 'TBR' not in dst_mask:
                    yield ('TBR', 0.500)
            elif 'TBR' in src_mask and 'TBR' not in dst_mask:
                yield ('TBR', 0.707)
            if 'TMR' not in src_mask and 'TFR' not in src_mask and 'TBR' not in src_mask:
                if 'FR' in src_mask:
                    yield ('FR', 1.000)
        case 'TBL':
            if 'TBL' in src_mask:
                yield ('TBL', 1.000)
            if 'TML' in src_mask and 'TML' not in dst_mask:
                yield ('TML', 0.707)
                if 'TFL' in src_mask and 'TFL' not in dst_mask:
                    yield ('TFL', 0.500)
            elif 'TFL' in src_mask and 'TFL' not in dst_mask:
                yield ('TBL', 0.707)
            if 'TML' not in src_mask and 'TFL' not in src_mask and 'TBL' not in src_mask:
                if 'BL' in src_mask:
                    yield ('BL', 1.000)
                if 'BC' in src_mask:
                    yield ('BC', 0.707)
        case 'TBR':
            if 'TBR' in src_mask:
                yield ('TBR', 1.000)
            if 'TMR' in src_mask and 'TMR' not in dst_mask:
                yield ('TMR', 0.707)
                if 'TFR' in src_mask and 'TFR' not in dst_mask:
                    yield ('TFR', 0.500)
            elif 'TFR' in src_mask and 'TFR' not in dst_mask:
                yield ('TBR', 0.707)
            if 'TMR' not in src_mask and 'TFR' not in src_mask and 'TBR' not in src_mask:
                if 'BR' in src_mask:
                    yield ('BR', 1.000)
                if 'BC' in src_mask:
                    yield ('BC', 0.707)
        case 'LFE':
            if 'LFE' in src_mask:
                yield ('LFE', 1.000)

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
const ChannelPositionName ChanPositionNames[{{ len(CHANS) }}] = {
{% for name, id in CHANS.items() %}
    { "{{ name }}", ChanPos_{{ id }} },
{% endfor %}
};

// Table of channel mask names.
const ChannelMaskName ChanMaskNames[{{ len(MASKS) }}] = {
{% for mask in MASKS.values() %}
    { "{{ mask.alias }}", ChanMask_Surround_{{ mask.id }} },
{% endfor %}
};

// Table of channel orders.
const ChannelOrderTable ChanOrderTables[{{ len(ORDERS) }}] = {
{% for id, chans in ORDERS.items() %}
    // ChanOrder_{{ id }}
    {
        {
{% for ch in chans %}
            ChanPos_{{ CHANS[ch] }},
{% endfor %}
            ChanPos_Max,
        },
    },
{% endfor %}
};

// Table of channel mappings.
const ChannelMapTable ChanMapTables[{{ len(all_mappings()) }}] = {
{% for src_idx, src_name, src_mask, dst_idx, dst_name, dst_mask in all_mappings() %}
{% if dst_idx == 0 %}
    // {{ src_name }}->...
{% endif %}
    {
        "{{ src_name }}->{{ dst_name }}",
        ChanMask_Surround_{{ src_mask.id }},
        ChanMask_Surround_{{ dst_mask.id }},
        {
{% for dst_chan in dst_mask.chans %}
            // {{ dst_chan }}
{% for src_chan, coeff in map_chan(dst_chan, dst_mask.chans, src_mask.chans) %}
            { ChanPos_{{ CHANS[dst_chan] }}, ChanPos_{{ CHANS[src_chan] }}, {{ "%.3f"|format(coeff) }}f },
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
