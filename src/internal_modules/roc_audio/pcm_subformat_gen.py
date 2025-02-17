#! /usr/bin/env python3
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

# generate initializers (C expressions) for min and max
# values for given pcm code
#
# e.g. for sint8:
#  min: -127 - 1
#  max: 127
def compute_minmax(code):
    suffix = ''
    if not code['is_signed']:
        suffix += 'u'
    if code['width'] >= 32:
        suffix += 'l'
    if code['width'] >= 64:
        suffix += 'l'

    if code['is_signed']:
        power = pow(2, code['width']-1) - 1

        min_val = f'-{power}{suffix} - 1'
        max_val = f'{power}{suffix}'
    else:
        power = pow(2, code['width']) - 1

        min_val = f'0{suffix}'
        max_val = f'{power}{suffix}'

    return min_val, max_val

# generate masks (C expressions) for given pcm code:
#  value_mask: masks all significant bits
#  sign_mask: masks sign bit
#  lsb_mask: masks all bits to the left of the sign bit
#
# e.g. for sint18_3:
#  value_mask: masks first 18 bits (0x3ffff)
#  sign_mask: masks 18th bit (0x20000)
#  lsb_mask: masks bits after 18th (0xfffc0000)
def compute_masks(code):
    value_mask = hex(int('1' * code['width'], 2))
    if not code['is_signed']:
        value_mask += 'u'

    if code['is_signed']:
        sign_mask = hex(int('1' + '0' * (code['width']-1), 2))
    else:
        sign_mask = None

    if code['is_signed'] and code['unpacked_width'] > code['width']:
        lsb_mask = hex(
            int(
                ('1' * (code['unpacked_width'] - code['width']) +
                '0' * code['width']),
                2))
    else:
        lsb_mask = None

    return value_mask, sign_mask, lsb_mask

# compute octet counts for given pcm code:
#  significant_octets: number of octets needed to fit all significant bits
#  packed_octets: number of octets needed to fit packed value
#  unpacked_octets: number of octets needed to fit unpacked value
#
# e.g. for sint18_3:
#  significant_octets: 3
#  packed_octets: 3
#  unpacked_octets: 4
def compute_octets(code):
    significant_octets = math.ceil(code['width'] / 8)
    packed_octets = math.ceil(code['packed_width'] / 8)
    unpacked_octets = math.ceil(code['unpacked_width'] / 8)

    return significant_octets, packed_octets, unpacked_octets

# generate enum name for pcm code + endian
# e.g.:
#  SInt18_3, Default => PcmSubformat_SInt18_3
#  SInt18_3, Little => PcmSubformat_SInt18_3_Le
#  SInt18_3, Big => PcmSubformat_SInt18_3_Be
def make_enum_name(code, endian):
    name = code['code']

    if endian != 'Default':
        if endian == 'Little':
            name += '_Le'
        if endian == 'Big':
            name += '_Be'

    return 'PcmSubformat_' + name

# generate short string name for pcm code + endian
# e.g.:
#  SInt18_3, Default => pcm_s18_3
#  SInt18_3, Little => pcm_s18_3le
#  SInt18_3, Big => pcm_s18_3be
def make_short_name(code, endian):
    name = code['short_name']

    if endian != 'Default':
        if not '_' in name:
            name += '_'
        if endian == 'Little':
            name += 'le'
        if endian == 'Big':
            name += 'be'

    return name

# generate format flags
def make_format_flags(code):
    flags = []

    if code['is_integer']:
        flags.append('IsInteger')
    else:
        flags.append('IsFloat')
    if code['is_signed']:
        flags.append('IsSigned')
    if code['width'] == code['packed_width']:
        flags.append('IsPacked')
    if code['width'] % 8 == 0:
        flags.append('IsAligned')

    return ' | '.join(['Pcm_'+f for f in flags])

# find all codes which short name has given prefix, and return
# sorted list of all possible characters in short name that can
# follow next to the prefix
#
# e.g. if prefix is ('f',), then codes starting with prefix
# are f32* and f64*, and nth_chars() returns ['3', '6']
def nth_chars(codes, prefix=()):
    pos = len(prefix)
    ret = set()
    for code in codes:
        name = code['short_name']
        if len(name) > pos and tuple(name[:pos]) == prefix:
            ret.add(name[pos])
    return list(sorted(ret))

CODES = [
    {
        'code': 'SInt8',
        'signed_code': 'SInt8',
        'type': 'int8_t',
        'signed_type': 'int8_t',
        'unsigned_type': 'uint8_t',
        'short_name': 's8',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 8,
        'width': 8,
        'packed_width': 8,
        'unpacked_width': 8,
    },
    {
        'code': 'UInt8',
        'signed_code': 'SInt8',
        'type': 'uint8_t',
        'signed_type': 'int8_t',
        'unsigned_type': 'uint8_t',
        'short_name': 'u8',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 8,
        'width': 8,
        'packed_width': 8,
        'unpacked_width': 8,
    },
    {
        'code': 'SInt16',
        'signed_code': 'SInt16',
        'type': 'int16_t',
        'signed_type': 'int16_t',
        'unsigned_type': 'uint16_t',
        'short_name': 's16',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 16,
        'width': 16,
        'packed_width': 16,
        'unpacked_width': 16,
    },
    {
        'code': 'UInt16',
        'signed_code': 'SInt16',
        'type': 'uint16_t',
        'signed_type': 'int16_t',
        'unsigned_type': 'uint16_t',
        'short_name': 'u16',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 16,
        'width': 16,
        'packed_width': 16,
        'unpacked_width': 16,
    },
    {
        'code': 'SInt18',
        'signed_code': 'SInt18',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's18',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 18,
        'width': 18,
        'packed_width': 18,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt18',
        'signed_code': 'SInt18',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u18',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 18,
        'width': 18,
        'packed_width': 18,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt18_3',
        'signed_code': 'SInt18_3',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's18_3',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 18,
        'width': 18,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt18_3',
        'signed_code': 'SInt18_3',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u18_3',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 18,
        'width': 18,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt18_4',
        'signed_code': 'SInt18_4',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's18_4',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 18,
        'width': 18,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt18_4',
        'signed_code': 'SInt18_4',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u18_4',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 18,
        'width': 18,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt20',
        'signed_code': 'SInt20',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's20',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 20,
        'width': 20,
        'packed_width': 20,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt20',
        'signed_code': 'SInt20',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u20',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 20,
        'width': 20,
        'packed_width': 20,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt20_3',
        'signed_code': 'SInt20_3',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's20_3',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 20,
        'width': 20,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt20_3',
        'signed_code': 'SInt20_3',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u20_3',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 20,
        'width': 20,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt20_4',
        'signed_code': 'SInt20_4',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's20_4',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 20,
        'width': 20,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt20_4',
        'signed_code': 'SInt20_4',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u20_4',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 20,
        'width': 20,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt24',
        'signed_code': 'SInt24',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's24',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 24,
        'width': 24,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt24',
        'signed_code': 'SInt24',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u24',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 24,
        'width': 24,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt24_4',
        'signed_code': 'SInt24_4',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's24_4',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 24,
        'width': 24,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt24_4',
        'signed_code': 'SInt24_4',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u24_4',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 24,
        'width': 24,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt32',
        'signed_code': 'SInt32',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 's32',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 32,
        'width': 32,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'UInt32',
        'signed_code': 'SInt32',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'short_name': 'u32',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 32,
        'width': 32,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'SInt64',
        'signed_code': 'SInt64',
        'type': 'int64_t',
        'signed_type': 'int64_t',
        'unsigned_type': 'uint64_t',
        'short_name': 's64',
        'is_integer': True,
        'is_signed': True,
        'is_raw': False,
        'depth': 64,
        'width': 64,
        'packed_width': 64,
        'unpacked_width': 64,
    },
    {
        'code': 'UInt64',
        'signed_code': 'SInt64',
        'type': 'uint64_t',
        'signed_type': 'int64_t',
        'unsigned_type': 'uint64_t',
        'short_name': 'u64',
        'is_integer': True,
        'is_signed': False,
        'is_raw': False,
        'depth': 64,
        'width': 64,
        'packed_width': 64,
        'unpacked_width': 64,
    },
    {
        'code': 'Float32',
        'signed_code': 'Float32',
        'type': 'float',
        'signed_type': 'float',
        'short_name': 'f32',
        'is_integer': False,
        'is_signed': True,
        'is_raw': True,
        'depth': 32,
        'width': 32,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'code': 'Float64',
        'signed_code': 'Float64',
        'type': 'double',
        'signed_type': 'double',
        'short_name': 'f64',
        'is_integer': False,
        'is_signed': True,
        'is_raw': False,
        'depth': 64,
        'width': 64,
        'packed_width': 64,
        'unpacked_width': 64,
    },
]

ENDIANS = [
    'Default',
    'Big',
    'Little',
]

TYPES = [
    ('int8_t', 1),
    ('uint8_t', 1),
    ('int16_t', 2),
    ('uint16_t', 2),
    ('int32_t', 4),
    ('uint32_t', 4),
    ('int64_t', 8),
    ('uint64_t', 8),
    ('float', 4),
    ('double', 8),
]

for code in CODES:
    code['min'] = f"pcm_{code['code'].lower()}_min"
    code['max'] = f"pcm_{code['code'].lower()}_max"

    code['signed_min'] = f"pcm_{code['signed_code'].lower()}_min"
    code['signed_max'] = f"pcm_{code['signed_code'].lower()}_max"

    code['min_value'], code['max_value'] = compute_minmax(code)

    code['value_mask'], code['sign_mask'], code['lsb_mask'] = \
      compute_masks(code)

    code['significant_octets'], code['packed_octets'], code['unpacked_octets'] = \
      compute_octets(code)

env = jinja2.Environment(
    trim_blocks=True,
    lstrip_blocks=True,
    undefined = jinja2.StrictUndefined)

template = env.from_string('''
/*
 * THIS FILE IS AUTO-GENERATED USING `pcm_subformat_gen.py'. DO NOT EDIT!
 */

#include "roc_audio/pcm_subformat.h"
#include "roc_audio/pcm_subformat_rw.h"
#include "roc_core/attributes.h"
#include "roc_core/cpu_traits.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

// PCM codes.
enum PcmCode {
{% for code in CODES %}
    PcmCode_{{ code.code }},
{% endfor %}
};

// PCM endians.
enum PcmEndian {
{% for endian in ENDIANS %}
{% if endian != 'Default' %}
    PcmEndian_{{ endian }},
{% endif %}
{% endfor %}
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    PcmEndian_Default = PcmEndian_Big,
#else
    PcmEndian_Default = PcmEndian_Little,
#endif
};

{% for code in CODES %}
{% if code.is_integer %}
// {{ code.code }} value range
const {{ code.type }} {{ code.min }} = {{ code.min_value }};
const {{ code.type }} {{ code.max }} = {{ code.max_value }};

{% endif %}
{% endfor %}
// Convert between signed and unsigned samples
template <PcmCode> struct pcm_sign_converter;

{% for code in CODES %}
{% if code.is_integer and not code.is_signed %}
// Convert {{ code.code }} from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_{{ code.code }}> {
    // {{ code.code }} from signed value
    static inline {{ code.unsigned_type }} from_signed({{ code.signed_type }} arg) {
        if (arg >= 0) {
            return {{ code.unsigned_type }}(arg) + {{ code.signed_max }} + 1;
        }
        return {{ code.unsigned_type }}(arg + {{ code.signed_max }} + 1);
    }

    // {{ code.code }} to signed value
    static inline {{ code.signed_type }} to_signed({{ code.unsigned_type }} arg) {
        if (arg >= {{ code.unsigned_type }}({{ code.signed_max }}) + 1) {
            return {{ code.signed_type }}(arg \
- {{ code.unsigned_type }}({{ code.signed_max }}) - 1);
        }
        return {{ code.signed_type }}(arg - {{ code.unsigned_type }}({{ code.signed_max }}) - 1);
    }
};

{% endif %}
{% endfor %}
// Convert between unpacked codes
template <PcmCode InCode, PcmCode OutCode> struct pcm_code_converter;

{% for ocode in CODES %}
{% for icode in CODES %}
{% if not icode.is_integer or not ocode.is_integer %}
// Convert {{ icode.code }} to {{ ocode.code }}
template <> struct pcm_code_converter<PcmCode_{{ icode.code }}, \
PcmCode_{{ ocode.code }}> {
    static inline {{ ocode.type }} convert({{ icode.type }} arg) {
{% if icode.code == ocode.code %}
        return arg;
{% else %}
{% if not icode.is_signed %}
        // convert to signed
        {{ icode.signed_type }} in = \
pcm_sign_converter<PcmCode_{{ icode.code }}>::to_signed(arg);
{% else %}
        {{ icode.type }} in = arg;
{% endif %}

        {{ ocode.signed_type }} out;
{% if not ocode.is_integer and not icode.is_integer %}
        // float to float
        out = {{ ocode.type }}(in);
{% elif not ocode.is_integer and icode.is_integer %}
        // integer to float
        out = {{ ocode.type }}(in * (1.0 / ((double){{ icode.signed_max }} + 1.0)));
{% elif ocode.is_integer and not icode.is_integer %}
        // float to integer
        const double d = double(in) * ((double){{ ocode.signed_max }} + 1.0);
        if (d < {{ ocode.signed_min }}) {
            // clip
            out = {{ ocode.signed_min }};
        } else if (d >= (double){{ ocode.signed_max }} + 1.0) {
            // clip
            out = {{ ocode.signed_max }};
        } else {
            out = {{ ocode.signed_type }}(d);
        }
{% endif %}

{% if not ocode.is_signed %}
        // convert to unsigned
        return pcm_sign_converter<PcmCode_{{ ocode.code }}>::from_signed(out);
{% else %}
        return out;
{% endif %}
{% endif %}
    }
};

{% endif %}
{% endfor %}
{% endfor %}
// N-byte native-endian sample
template <class T> struct pcm_sample;

{% for type, size in TYPES %}
// {{ type }} native-endian sample
template <> struct pcm_sample<{{ type }}> {
    union {
        {{ type }} value;
        ROC_ATTR_PACKED_BEGIN struct {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
{% for n in reversed(range(size)) %}
            uint8_t octet{{ n }};
{% endfor %}
#else
{% for n in range(size) %}
            uint8_t octet{{ n }};
{% endfor %}
#endif
        } ROC_ATTR_PACKED_END octets;
    };
};

{% endfor %}
// Sample packer / unpacker
template <PcmCode, PcmEndian> struct pcm_packer;

{% for code in CODES %}
{% for endian in ['Big', 'Little'] %}
// {{ code.code }} {{ endian }}-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_{{ code.code }}, PcmEndian_{{ endian }}> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, {{ code.type }} arg) {
        // native-endian view of octets
        pcm_sample<{{ code.type }}> p;
        p.value = arg;

{% if code.width < code.packed_width %}
        // zeroise padding bits
        p.value &= {{ code.value_mask }};

{% endif %}
        // write in {{ endian.lower() }}-endian order
{% for n in range(code.packed_octets) %}
{% if endian == 'Big' %}
{% set n = code.packed_octets - n - 1 %}
{% endif %}
{% if code.packed_width % 8 == 0 %}
        pcm_aligned_write(buffer, bit_offset, p.octets.octet{{ n }});
{% else %}
        pcm_unaligned_write(buffer, bit_offset, \
{{ code.packed_width % 8 if n == code.packed_octets-1 else 8 }}, p.octets.octet{{ n }});
{% endif %}
{% endfor %}
    }

    // Unpack next sample from buffer
    static inline {{ code.type }} unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<{{ code.type }}> p;

        // read in {{ endian.lower() }}-endian order
{% for n in range(code.unpacked_octets) %}
{% if endian == 'Big' %}
{% set n = code.unpacked_octets - n - 1 %}
{% endif %}
{% if n >= code.packed_octets %}
        p.octets.octet{{ n }} = 0;
{% elif code.packed_width % 8 == 0 %}
        p.octets.octet{{ n }} = pcm_aligned_read(buffer, bit_offset);
{% else %}
        p.octets.octet{{ n }} = pcm_unaligned_read(buffer, bit_offset, \
{{ code.packed_width % 8 if n == code.packed_octets-1 else 8 }});
{% endif %}
{% endfor %}

{% if code.width < code.packed_width %}
        // zeroise padding bits
        p.value &= {{ code.value_mask }};

{% endif %}
{% if code.is_signed and code.width < code.unpacked_width %}
        if (p.value & {{ code.sign_mask }}) {
            // sign extension
            p.value |= ({{ code.type }}){{ code.lsb_mask }};
        }

{% endif %}
        return p.value;
    }
};

{% endfor %}
{% endfor %}
// Mapping function implementation
template <PcmCode InCode, PcmEndian InEndian, PcmCode OutCode, PcmEndian OutEndian>
struct pcm_mapper {
    static void map(const uint8_t* in_data,
                    size_t& in_bit_off,
                    uint8_t* out_data,
                    size_t& out_bit_off,
                    size_t n_samples) {
        for (size_t n = 0; n < n_samples; n++) {
            pcm_packer<OutCode, OutEndian>::pack(
                out_data, out_bit_off,
                pcm_code_converter<InCode, OutCode>::convert(
                    pcm_packer<InCode, InEndian>::unpack(in_data, in_bit_off)));
        }
    }
};

// Select mapping function
template <PcmCode InCode, PcmEndian InEndian>
PcmMapFn pcm_map_to_raw(PcmSubformat raw_format) {
    switch (raw_format) {
{% for ocode in CODES %}
{% if ocode.is_raw: %}
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    case {{ make_enum_name(ocode, 'Default') }}:
    case {{ make_enum_name(ocode, 'Big') }}:
#else
    case {{ make_enum_name(ocode, 'Default') }}:
    case {{ make_enum_name(ocode, 'Little') }}:
#endif
        return &pcm_mapper<InCode, InEndian, PcmCode_{{ ocode.code }}, PcmEndian_Default>::map;
{% endif %}
{% endfor %}
    default:
        break;
    }
    return NULL;
}

// Select mapping function
template <PcmCode OutCode, PcmEndian OutEndian>
PcmMapFn pcm_map_from_raw(PcmSubformat raw_format) {
    switch (raw_format) {
{% for icode in CODES %}
{% if icode.is_raw: %}
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    case {{ make_enum_name(icode, 'Default') }}:
    case {{ make_enum_name(icode, 'Big') }}:
#else
    case {{ make_enum_name(icode, 'Default') }}:
    case {{ make_enum_name(icode, 'Little') }}:
#endif
        return &pcm_mapper<PcmCode_{{ icode.code }}, PcmEndian_Default, OutCode, OutEndian>::map;
{% endif %}
{% endfor %}
    default:
        break;
    }
    return NULL;
}

} // namespace

// Select mapping function
PcmMapFn pcm_subformat_mapfn(PcmSubformat in_format, PcmSubformat out_format) {
    // non-raw to raw
    switch (in_format) {
{% for icode in CODES %}
{% if icode.is_raw %}
#if ROC_CPU_ENDIAN != ROC_CPU_BE
    case {{ make_enum_name(icode, 'Big') }}:
        return pcm_map_to_raw<PcmCode_{{ icode.code }}, PcmEndian_Big>(out_format);
#else
    case {{ make_enum_name(icode, 'Little') }}:
        return pcm_map_to_raw<PcmCode_{{ icode.code }}, PcmEndian_Little>(out_format);
#endif
{% else %}
{% for iendian in ENDIANS %}
    case {{ make_enum_name(icode, iendian) }}:
        return pcm_map_to_raw<PcmCode_{{ icode.code }}, PcmEndian_{{ iendian }}>(out_format);
{% endfor %}
{% endif %}
{% endfor %}
    default:
        break;
    }

    // raw to non-raw
    switch (out_format) {
{% for ocode in CODES %}
{% if ocode.is_raw %}
#if ROC_CPU_ENDIAN != ROC_CPU_BE
    case {{ make_enum_name(ocode, 'Big') }}:
        return pcm_map_from_raw<PcmCode_{{ ocode.code }}, PcmEndian_Big>(in_format);
#else
    case {{ make_enum_name(ocode, 'Little') }}:
        return pcm_map_from_raw<PcmCode_{{ ocode.code }}, PcmEndian_Little>(in_format);
#endif
{% else %}
{% for oendian in ENDIANS %}
    case {{ make_enum_name(ocode, oendian) }}:
        return pcm_map_from_raw<PcmCode_{{ ocode.code }}, PcmEndian_{{ oendian }}>(in_format);
{% endfor %}
{% endif %}
{% endfor %}
    default:
        break;
    }

    // raw to raw
    switch (out_format) {
{% for ocode in CODES %}
{% if ocode.is_raw %}
    case {{ make_enum_name(ocode, 'Default') }}:
        return pcm_map_from_raw<PcmCode_{{ ocode.code }}, PcmEndian_Default>(in_format);
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    case {{ make_enum_name(ocode, 'Big') }}:
        return pcm_map_from_raw<PcmCode_{{ ocode.code }}, PcmEndian_Default>(in_format);
#else
    case {{ make_enum_name(ocode, 'Little') }}:
        return pcm_map_from_raw<PcmCode_{{ ocode.code }}, PcmEndian_Default>(in_format);
#endif
{% endif %}
{% endfor %}
    default:
        break;
    }

    return NULL;
}

// Get format traits
PcmTraits pcm_subformat_traits(PcmSubformat format) {
    PcmTraits traits;

    switch (format) {
{% for code in CODES %}
{% for endian in ENDIANS %}
    case {{ make_enum_name(code, endian) }}:
        traits.id = {{ make_enum_name(code, endian) }};
        traits.name = "{{ make_short_name(code, endian) }}";
        traits.bit_width = {{ code.packed_width }};
        traits.bit_depth = {{ code.depth }};
        traits.flags = {{ make_format_flags(code) }};
{% if endian == 'Default' %}
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = {{ make_enum_name(code, 'Big') }};
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = {{ make_enum_name(code, 'Little') }};
#endif
        traits.native_alias = {{ make_enum_name(code, 'Default') }};
{% elif endian == 'Big' %}
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = {{ make_enum_name(code, 'Default') }};
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = {{ make_enum_name(code, 'Big') }};
#endif
        traits.portable_alias = {{ make_enum_name(code, endian) }};
{% elif endian == 'Little' %}
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = {{ make_enum_name(code, 'Default') }};
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = {{ make_enum_name(code, 'Little') }};
#endif
        traits.portable_alias = {{ make_enum_name(code, endian) }};
{% endif %}
        traits.default_variant = {{ make_enum_name(code, 'Default') }};
        traits.be_variant = {{ make_enum_name(code, 'Big') }};
        traits.le_variant = {{ make_enum_name(code, 'Little') }};
        break;

{% endfor %}
{% endfor %}
    default:
        break;
    }

    return traits;
}

const char* pcm_subformat_to_str(PcmSubformat format) {
    switch (format) {
{% for code in CODES %}
{% for endian in ENDIANS %}
    case {{ make_enum_name(code, endian) }}:
        return "{{ make_short_name(code, endian) }}";
{% endfor %}
{% endfor %}
    default:
        break;
    }
    return NULL;
}

PcmSubformat pcm_subformat_from_str(const char* str) {
    if (!str) {
        return PcmSubformat_Invalid;
    }
{% for c0 in nth_chars(CODES) %}
    if (str[0] == '{{ c0 }}') {
{% for c1 in nth_chars(CODES, (c0,)) %}
        if (str[1] == '{{ c1 }}') {
{% for c2 in nth_chars(CODES, (c0, c1)) %}
            if (str[2] == '{{ c2 }}') {
{% for code in CODES %}
{% if tuple(code.short_name[:3]) == (c0, c1, c2) %}
{% for endian in ENDIANS %}
                if (strcmp(str, "{{ make_short_name(code, endian) }}") == 0) {
                    return {{ make_enum_name(code, endian) }};
                }
{% endfor %}
{% endif %}
{% endfor %}
                return PcmSubformat_Invalid;
            }
{% endfor %}
{% for code in CODES %}
{% if tuple(code.short_name) == (c0, c1) %}
{% for endian in ENDIANS %}
            if (strcmp(str, "{{ make_short_name(code, endian) }}") == 0) {
                return {{ make_enum_name(code, endian) }};
            }
{% endfor %}
{% endif %}
{% endfor %}
            return PcmSubformat_Invalid;
        }
{% endfor %}
        return PcmSubformat_Invalid;
    }
{% endfor %}
    return PcmSubformat_Invalid;
}

} // namespace audio
} // namespace roc
'''.strip())

text = template.render(
    **dict(list(globals().items()) + list(builtins.__dict__.items())),
    )

os.chdir(os.path.dirname(os.path.abspath(__file__)))

with open('pcm_subformat.cpp', 'w') as fp:
    print(text, file=fp)
