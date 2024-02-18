#! /usr/bin/env python3
from collections import OrderedDict
from itertools import islice
import builtins
import math
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

def skip_all(mask):
    return True

def skip_all_except(*masks):
    def fn(mask):
        return mask not in masks
    return fn

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
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('1.1', {
        'id':        '1_1',
        'chans':     'FC LFE'.split(),
     }),
    ('1.1-3c', {
        'id':        '1_1_3c',
        'chans':     'FLC FC FRC LFE'.split(),
     }),
    # 2.x
    ('2.0', {
        'id':        'Stereo',
        'text_name': 'stereo',
        'chans':     'FL FR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('2.1', {
        'id':        '2_1',
        'text_name': 'surround2.1',
        'chans':     'FL FR LFE'.split(),
     }),
    # 3.x
    ('3.0', {
        'id':        '3_0',
        'text_name': 'surround3.0',
        'chans':     'FL FC FR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('3.1', {
        'id':        '3_1',
        'text_name': 'surround3.1',
        'chans':     'FL FC FR LFE'.split(),
     }),
    ('3.1-3c', {
        'id':        '3_1_3c',
        'chans':     'FL FLC FC FRC FR LFE'.split(),
     }),
    # 4.x
    ('4.0', {
        'id':        '4_0',
        'text_name': 'surround4.0',
        'chans':     'FL FR BL BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('4.1', {
        'id':        '4_1',
        'text_name': 'surround4.1',
        'chans':     'FL FR BL BR LFE'.split(),
     }),
    # 5.x
    ('5.0', {
        'id':        '5_0',
        'text_name': 'surround5.0',
        'chans':     'FL FC FR BL BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('5.1', {
        'id':        '5_1',
        'text_name': 'surround5.1',
        'chans':     'FL FC FR BL BR LFE'.split(),
        'skip_from':  skip_all_except('5.1.2', '5.1.2-3c', '5.1.4', '5.1.4-3c'),
        'skip_to':    skip_all,
     }),
    ('5.1-3c', {
        'id':        '5_1_3c',
        'chans':     'FL FLC FC FRC FR BL BR LFE'.split(),
        'skip_from':  skip_all_except('5.1.2', '5.1.2-3c', '5.1.4', '5.1.4-3c'),
        'skip_to':    skip_all,
     }),
    ('5.1.2', {
        'id':        '5_1_2',
        'text_name': 'surround5.1.2',
        'chans':     'FL FC FR BL BR TML TMR LFE'.split(),
     }),
    ('5.1.2-3c', {
        'id':        '5_1_2_3c',
        'chans':     'FL FLC FC FRC FR BL BR TML TMR LFE'.split(),
     }),
    ('5.1.4', {
        'id':        '5_1_4',
        'text_name': 'surround5.1.4',
        'chans':     'FL FC FR BL BR TFL TFR TBL TBR LFE'.split(),
     }),
    ('5.1.4-3c', {
        'id':        '5_1_4_3c',
        'chans':     'FL FLC FC FRC FR BL BR TFL TFR TBL TBR LFE'.split(),
     }),
    # 6.x
    ('6.0', {
        'id':        '6_0',
        'text_name': 'surround6.0',
        'chans':     'FL FC FR BL BC BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('6.1', {
        'id':        '6_1',
        'text_name': 'surround6.1',
        'chans':     'FL FC FR BL BC BR LFE'.split(),
     }),
    ('6.1-3c', {
        'id':        '6_1_3c',
        'chans':     'FL FLC FC FRC FR BL BC BR LFE'.split(),
     }),
    # 7.x
    ('7.0', {
        'id':        '7_0',
        'text_name': 'surround7.0',
        'chans':     'FL FC FR SL SR BL BR'.split(),
        'skip_from':  skip_all,
        'skip_to':    skip_all,
     }),
    ('7.1', {
        'id':        '7_1',
        'text_name': 'surround7.1',
        'chans':     'FL FC FR SL SR BL BR LFE'.split(),
        'skip_from':  skip_all_except('7.1.2', '7.1.2-3c', '7.1.4', '7.1.4-3c'),
        'skip_to':    skip_all,
     }),
    ('7.1-3c', {
        'id':        '7_1_3c',
        'chans':     'FL FLC FC FRC FR SL SR BL BR LFE'.split(),
        'skip_from':  skip_all_except('7.1.2', '7.1.2-3c', '7.1.4', '7.1.4-3c'),
        'skip_to':    skip_all,
     }),
    ('7.1.2', {
        'id':        '7_1_2',
        'text_name': 'surround7.1.2',
        'chans':     'FL FC FR SL SR BL BR TML TMR LFE'.split(),
     }),
    ('7.1.2-3c', {
        'id':        '7_1_2_3c',
        'chans':     'FL FLC FC FRC FR SL SR BL BR TML TMR LFE'.split(),
     }),
    ('7.1.4', {
        'id':        '7_1_4',
        'text_name': 'surround7.1.4',
        'chans':     'FL FC FR SL SR BL BR TFL TFR TBL TBR LFE'.split(),
     }),
    ('7.1.4-3c', {
        'id':        '7_1_4_3c',
        'chans':     'FL FLC FC FRC FR SL SR BL BR TFL TFR TBL TBR LFE'.split(),
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
    # Returns True if we should skip mapping src_mask => dst_mask.
    def skip_mapping(src_name, src_mask, dst_name, dst_mask):
        if 'skip_from' in dst_mask:
            if dst_mask['skip_from'](src_name):
                return True
        if 'skip_to' in src_mask:
            if src_mask['skip_to'](dst_name):
                return True
        return False

    ret = []
    for src_idx, (src_name, src_mask) in islice(enumerate(MASKS.items()), 1, None):
        src_start = True
        for dst_idx, (dst_name, dst_mask) in islice(enumerate(MASKS.items()), 0, src_idx):
            if not skip_mapping(src_name, src_mask, dst_name, dst_mask):
                ret += [(src_start, src_name, src_mask, dst_name, dst_mask)]
                src_start = False

    return ret

# For given dst chan, returns list of src chans with coefficients.
# E.g. for 'FC' it can return [('FL', 1.000), ('FR', 1.000)], depending
# on src mask and dst mask.
# This function was designed to produce downmixing tables equivalent to those
# defined in ITU and AC-3 standards, referenced in channel_tables.h.
# It, presumably, replicates heuristics used to define the tables,
# and allows to extend their usage to combinations not covered in documents.
def map_chan(dst_chan, dst_mask, src_mask):
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
    match dst_chan:
        case 'FL':
            # mix front
            if 'FL' in src_mask:
                yield ('FL', 1.000)
            if 'FLC' in src_mask and 'FLC' not in dst_mask:
                yield ('FLC', 0.707)
            if 'FC' in src_mask and 'FC' not in dst_mask:
                yield ('FC', 0.707)
            # mix surround
            if 'SL' in src_mask and ('BL' not in dst_mask and 'SL' not in dst_mask):
                yield ('SL', 0.707)
            if 'BL' in src_mask and ('BL' not in dst_mask and 'SL' not in dst_mask):
                yield ('BL', 0.707)
            if 'BC' in src_mask and ('BL' not in dst_mask and 'SL' not in dst_mask
                                     and 'BC' not in dst_mask):
                yield ('BC', 0.500)
            # mix top
            if 'TFL' in src_mask and 'TFL' not in dst_mask:
                yield ('TFL', 0.707)
            if 'TML' in src_mask and ('TML' not in dst_mask and 'TFL' not in dst_mask and
                                      'SL' not in dst_mask):
                yield ('TML', 0.707)
            if 'TBL' in src_mask and ('TBL' not in dst_mask and 'BL' not in dst_mask):
                yield ('TBL', 0.500)
        case 'FR':
            # symmetrical to FL
            yield from r_from_l()
        case 'FLC':
            # mix FLC or FC
            if 'FLC' in src_mask:
                yield ('FLC', 1.000)
            elif 'FC' in src_mask:
                yield ('FC', 0.707)
            # mix left-side channels that we would mix into FC if there were
            # no FL/FR in destination (so we'd have to mix left surround into FC)
            for chan, coeff in map_chan(
                'FC',
                [chan for chan in dst_mask if chan not in ('FL', 'FR')],
                src_mask):
                if is_l(chan):
                    yield (chan, coeff)
        case 'FRC':
            # symmetrical to FLC
            yield from r_from_l()
        case 'FC':
            # mix front
            if 'FL' in src_mask and ('FL' not in dst_mask or 'FC' not in src_mask or
                                     ('BC' in src_mask and 'BL' not in dst_mask)):
                yield ('FL', 0.707)
            if 'FLC' in src_mask and 'FLC' not in dst_mask:
                yield ('FLC', 0.707)
            if 'FC' in src_mask:
                yield ('FC', 1.000)
            if 'FRC' in src_mask and 'FRC' not in dst_mask:
                yield ('FRC', 0.707)
            if 'FR' in src_mask and ('FR' not in dst_mask or 'FC' not in src_mask or
                                     ('BC' in src_mask and 'BR' not in dst_mask)):
                yield ('FR', 0.707)
            # mix surround
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
            # mix top
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
        case 'SL':
            # mix surround
            if 'SL' in src_mask:
                yield ('SL', 1.000)
            # mix top
            if 'TML' in src_mask and 'TML' not in dst_mask:
                yield ('TML', 0.707)
        case 'SR':
            # symmetrical to SL
            yield from r_from_l()
        case 'BL':
            # mix surrounf
            if 'SL' in src_mask and 'SL' not in dst_mask:
                yield ('SL', 1.000)
            if 'BL' in src_mask:
                yield ('BL', 1.000)
            if 'BC' in src_mask and 'BC' not in dst_mask:
                yield ('BC', 0.707)
            # mix top
            if 'TML' in src_mask and ('TML' not in dst_mask and 'TFL' not in dst_mask and
                                      'SL' not in dst_mask):
                yield ('TML', 0.707)
            if 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
        case 'BR':
            # symmetrical to BL
            yield from r_from_l()
        case 'BC':
            # mix surround
            if 'BL' in src_mask and ('BL' not in dst_mask or 'BC' not in src_mask):
                yield ('BL', 1.000)
            if 'BC' in src_mask:
                yield ('BC', 1.000)
            if 'BR' in src_mask and ('BR' not in dst_mask or 'BC' not in src_mask):
                yield ('BR', 1.000)
            if 'SL' in src_mask and 'SL' not in dst_mask:
                yield ('SL', 1.000)
            if 'SR' in src_mask and 'SR' not in dst_mask:
                yield ('SR', 1.000)
            # mix top
            if 'TML' in src_mask and ('TML' not in dst_mask and 'TBL' not in dst_mask):
                yield ('TML', 0.707)
            if 'TMR' in src_mask and ('TMR' not in dst_mask and 'TBR' not in dst_mask):
                yield ('TMR', 0.707)
            if 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
            if 'TBR' in src_mask and 'TBR' not in dst_mask:
                yield ('TBR', 0.707)
        case 'TML':
            # mix top
            if 'TML' in src_mask:
                yield ('TML', 1.000)
            if 'TFL' in src_mask and 'TFL' not in dst_mask:
                yield ('TFL', 0.707)
            if 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
            # mix front & back
            if 'TML' not in src_mask and 'TFL' not in src_mask and 'TBL' not in src_mask:
                if 'FL' in src_mask:
                    yield ('FL', 1.000)
                if 'BL' in src_mask:
                    yield ('BL', 1.000)
                if 'BC' in src_mask:
                    yield ('BC', 0.707)
        case 'TMR':
            # symmetrical to TML
            yield from r_from_l()
        case 'TFL':
            # mix top
            if 'TFL' in src_mask:
                yield ('TFL', 1.000)
            if 'TML' in src_mask and 'TML' not in dst_mask:
                yield ('TML', 0.707)
                if 'TBL' in src_mask and 'TBL' not in dst_mask:
                    yield ('TBL', 0.500)
            elif 'TBL' in src_mask and 'TBL' not in dst_mask:
                yield ('TBL', 0.707)
            # mix front
            if 'TML' not in src_mask and 'TFL' not in src_mask and 'TBL' not in src_mask:
                if 'FL' in src_mask:
                    yield ('FL', 1.000)
        case 'TFR':
            # symmetrical to TFR
            yield from r_from_l()
        case 'TBL':
            # mix top
            if 'TBL' in src_mask:
                yield ('TBL', 1.000)
            if 'TML' in src_mask and 'TML' not in dst_mask:
                yield ('TML', 0.707)
                if 'TFL' in src_mask and 'TFL' not in dst_mask:
                    yield ('TFL', 0.500)
            elif 'TFL' in src_mask and 'TFL' not in dst_mask:
                yield ('TBL', 0.707)
            # mix back
            if 'TML' not in src_mask and 'TFL' not in src_mask and 'TBL' not in src_mask:
                if 'BL' in src_mask:
                    yield ('BL', 1.000)
                if 'BC' in src_mask:
                    yield ('BC', 0.707)
        case 'TBR':
            # symmetrical to TBR
            yield from r_from_l()
        case 'LFE':
            # LFE is just copied
            if 'LFE' in src_mask:
                yield ('LFE', 1.000)
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
