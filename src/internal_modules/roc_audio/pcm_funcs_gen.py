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
#  value_masks: masks first 18 bits (0x3ffff)
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

# generate short name for pcm code + endianess
# e.g.:
#  sint18_3, native => s18_3
#  sint18_3, little => s18_3le
#  sint18_3, big => s18_3be
def short_name(code, endian):
    name = code['short_name']

    if endian != 'Native':
        if not '_' in name:
            name += '_'
        if endian == 'Little':
            name += 'le'
        if endian == 'Big':
            name += 'be'

    return name

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
        'width': 64,
        'packed_width': 64,
        'unpacked_width': 64,
    },
]

ENDIANS = [
    'Native',
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
 * THIS FILE IS AUTO-GENERATED USING `pcm_funcs_gen.py'. DO NOT EDIT!
 */

#ifndef ROC_AUDIO_PCM_FUNCS_H_
#define ROC_AUDIO_PCM_FUNCS_H_

#include "roc_audio/pcm_format.h"
#include "roc_core/attributes.h"
#include "roc_core/cpu_traits.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

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
{% if code.is_integer %}
// Convert {{ code.code }} from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_{{ code.code }}> {
{% if code.is_signed %}
    // {{ code.code }} from unsigned value
    static inline {{ code.signed_type }} from_unsigned({{ code.unsigned_type }} arg) {
        if (arg < {{ code.unsigned_type }}({{ code.signed_max }}) + 1) {
            return {{ code.signed_type }}(arg) - {{ code.signed_max }} - 1;
        }
        return {{ code.signed_type }}(arg - {{ code.unsigned_type }}({{ code.signed_max }}) - 1);
    }

    // {{ code.code }} to unsigned value
    static inline {{ code.unsigned_type }} to_unsigned({{ code.signed_type }} arg) {
        if (arg >= 0) {
            return {{ code.unsigned_type }}(arg) + {{ code.signed_max }} + 1;
        }
        return {{ code.unsigned_type }}(arg + {{ code.signed_max }} + 1);
    }
{% else %}
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
{% endif %}
};

{% endif %}
{% endfor %}
// Convert between unpacked CODES
template <PcmCode InCode, PcmCode OutCode> struct pcm_code_converter;

{% for out in CODES %}
{% for in in CODES %}
{% set both_unsigned = not in.is_signed and not out.is_signed %}
// Convert {{ in.code }} to {{ out.code }}
template <> struct pcm_code_converter<PcmCode_{{ in.code }}, \
PcmCode_{{ out.code }}> {
    static inline {{ out.type }} convert({{ in.type }} arg) {
{% if in.code == out.code %}
        return arg;
{% else %}
{% if not in.is_signed and not both_unsigned %}
        // convert to signed
        {{ in.signed_type }} in = \
pcm_sign_converter<PcmCode_{{ in.code }}>::to_signed(arg);
{% else %}
        {{ in.type }} in = arg;
{% endif %}

{% if both_unsigned %}
        {{ out.type }} out;
{% else %}
        {{ out.signed_type }} out;
{% endif %}
{% if not out.is_integer and not in.is_integer %}
        // float to float
        out = {{ out.type }}(in);
{% elif not out.is_integer and in.is_integer %}
        // integer to float
        out = {{ out.type }}(in * (1.0 / ((double){{ in.signed_max }} + 1.0)));
{% elif out.is_integer and not in.is_integer %}
        // float to integer
        const double d = double(in) * ((double){{ out.signed_max }} + 1.0);
        if (d < {{ out.signed_min }}) {
            // clip
            out = {{ out.signed_min }};
        } else if (d >= (double){{ out.signed_max }} + 1.0) {
            // clip
            out = {{ out.signed_max }};
        } else {
            out = {{ out.signed_type }}(d);
        }
{% elif out.width == in.width %}
        out = in;
{% elif out.width < in.width and both_unsigned %}
        // downscale unsigned integer
        out = {{ out.type }}(in >> {{ in.width - out.width }});
{% elif out.width < in.width and not both_unsigned %}
        // downscale signed integer
        if (in > {{ in.signed_type }}({{ in.signed_max }} - \
({{ in.signed_type }}(1) << {{ in.width - out.width - 1 }}))) {
            // clip
            out = {{ out.signed_max }};
        } else {
            out = {{ out.signed_type }}({{ in.unsigned_type }}(in + \
({{ in.signed_type }}(1) << {{ in.width - out.width - 1 }})) >> {{ in.width - out.width }});
        }
{% elif out.width > in.width and both_unsigned %}
        // upscale unsigned integer
        out = {{ out.type }}({{ out.type }}(in) << {{ out.width - in.width }});
{% elif out.width > in.width and not both_unsigned %}
        // upscale signed integer
        out = {{ out.signed_type }}({{ out.unsigned_type }}(in) << {{ out.width - in.width }});
{% endif %}

{% if not out.is_signed and not both_unsigned %}
        // convert to unsigned
        return pcm_sign_converter<PcmCode_{{ out.code }}>::from_signed(out);
{% else %}
        return out;
{% endif %}
{% endif %}
    }
};

{% endfor %}
{% endfor %}
// N-byte native-endian packed octet array
template <size_t N> struct pcm_octets;

{% for size in [1, 2, 4, 8] %}
// {{ size }}-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<{{ size }}> {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
{% for n in reversed(range(size)) %}
    uint8_t octet{{ n }};
{% endfor %}
#else
{% for n in range(size) %}
    uint8_t octet{{ n }};
{% endfor %}
#endif
} ROC_ATTR_PACKED_END;

{% endfor %}
// N-byte native-endian sample
template <class T> struct pcm_sample;

{% for type, size in TYPES %}
// {{ type }} native-endian sample
template <> struct pcm_sample<{{ type }}> {
    union {
        {{ type }} value;
        pcm_octets<{{ size }}> octets;
    };
};

{% endfor %}
// Write octet at given byte-aligned bit offset
inline void pcm_aligned_write(uint8_t* buffer, size_t& bit_offset, uint8_t arg) {
    buffer[bit_offset >> 3] = arg;
    bit_offset += 8;
}

// Read octet at given byte-aligned bit offset
inline uint8_t pcm_aligned_read(const uint8_t* buffer, size_t& bit_offset) {
    uint8_t ret = buffer[bit_offset >> 3];
    bit_offset += 8;
    return ret;
}

// Write value (at most 8 bits) at given unaligned bit offset
inline void
pcm_unaligned_write(uint8_t* buffer, size_t& bit_offset, size_t bit_length, uint8_t arg) {
    size_t byte_index = (bit_offset >> 3);
    size_t bit_index = (bit_offset & 0x7u);

    if (bit_index == 0) {
        buffer[byte_index] = 0;
    }

    buffer[byte_index] |= uint8_t(arg << (8 - bit_length) >> bit_index);

    if (bit_index + bit_length > 8) {
        buffer[byte_index + 1] = uint8_t(arg << bit_index);
    }

    bit_offset += bit_length;
}

// Read value (at most 8 bits) at given unaligned bit offset
inline uint8_t
pcm_unaligned_read(const uint8_t* buffer, size_t& bit_offset, size_t bit_length) {
    size_t byte_index = (bit_offset >> 3);
    size_t bit_index = (bit_offset & 0x7u);

    uint8_t ret = uint8_t(buffer[byte_index] << bit_index >> (8 - bit_length));

    if (bit_index + bit_length > 8) {
        ret |= uint8_t(buffer[byte_index + 1] >> (8 - bit_index) >> (8 - bit_length));
    }

    bit_offset += bit_length;
    return ret;
}

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
// Map code and endian of samples
template <PcmCode InCode, PcmCode OutCode, PcmEndian InEndian, PcmEndian OutEndian>
struct pcm_mapper {
    static inline void map(const uint8_t* in_data,
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

// Mapping function
typedef void (*pcm_map_func_t)(
    const uint8_t* in_data,
    size_t& in_bit_off,
    uint8_t* out_data,
    size_t& out_bit_off,
    size_t n_samples);

// Select mapping function
template <PcmCode InCode, PcmCode OutCode, PcmEndian InEndian, PcmEndian OutEndian>
pcm_map_func_t pcm_map_func() {
    return &pcm_mapper<InCode, OutCode, InEndian, OutEndian>::map;
}

// Select mapping function
template <PcmCode InCode, PcmCode OutCode, PcmEndian InEndian>
pcm_map_func_t pcm_map_func(PcmEndian out_endian) {
    switch (out_endian) {
{% for endian in ENDIANS %}
    case PcmEndian_{{ endian }}:
{% if endian == 'Native' %}
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        return pcm_map_func<InCode, OutCode, InEndian, PcmEndian_Big>();
#else
        return pcm_map_func<InCode, OutCode, InEndian, PcmEndian_Little>();
#endif
{% else %}
        return pcm_map_func<InCode, OutCode, InEndian, PcmEndian_{{ endian }}>();
{% endif %}
{% endfor %}
    case PcmEndian_Max:
        break;
    }
    return NULL;
}

// Select mapping function
template <PcmCode InCode, PcmCode OutCode>
pcm_map_func_t pcm_map_func(PcmEndian in_endian, PcmEndian out_endian) {
    switch (in_endian) {
{% for endian in ENDIANS %}
    case PcmEndian_{{ endian }}:
{% if endian == 'Native' %}
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        return pcm_map_func<InCode, OutCode, PcmEndian_Big>(out_endian);
#else
        return pcm_map_func<InCode, OutCode, PcmEndian_Little>(out_endian);
#endif
{% else %}
        return pcm_map_func<InCode, OutCode, PcmEndian_{{ endian }}>(out_endian);
{% endif %}
{% endfor %}
    case PcmEndian_Max:
        break;
    }
    return NULL;
}

// Select mapping function
template <PcmCode InCode>
inline pcm_map_func_t pcm_map_func(PcmCode out_code,
                                   PcmEndian in_endian,
                                   PcmEndian out_endian) {
    switch (out_code) {
{% for code in CODES %}
    case PcmCode_{{ code.code }}:
        return pcm_map_func<InCode, PcmCode_{{ code.code }}>(in_endian, out_endian);
{% endfor %}
    case PcmCode_Max:
        break;
    }
    return NULL;
}

// Select mapping function
inline pcm_map_func_t pcm_map_func(PcmCode in_code,
                                   PcmCode out_code,
                                   PcmEndian in_endian,
                                   PcmEndian out_endian) {
    switch (in_code) {
{% for code in CODES %}
    case PcmCode_{{ code.code }}:
        return pcm_map_func<PcmCode_{{ code.code }}>(out_code, \
in_endian, out_endian);
{% endfor %}
    case PcmCode_Max:
        break;
    }
    return NULL;
}

// Get number of meaningful bits per sample
inline size_t pcm_bit_depth(PcmCode code) {
    switch (code) {
{% for code in CODES %}
    case PcmCode_{{ code.code }}:
        return {{ code.width }};
{% endfor %}
    case PcmCode_Max:
        break;
    }
    return 0;
}

// Get number of total bits per sample
inline size_t pcm_bit_width(PcmCode code) {
    switch (code) {
{% for code in CODES %}
    case PcmCode_{{ code.code }}:
        return {{ code.packed_width }};
{% endfor %}
    case PcmCode_Max:
        break;
    }
    return 0;
}

// Check if code is integer
inline size_t pcm_is_integer(PcmCode code) {
    switch (code) {
{% for code in CODES %}
    case PcmCode_{{ code.code }}:
        return {{ str(code.is_integer).lower() }};
{% endfor %}
    case PcmCode_Max:
        break;
    }
    return false;
}

// Check if code is signed
inline size_t pcm_is_signed(PcmCode code) {
    switch (code) {
{% for code in CODES %}
    case PcmCode_{{ code.code }}:
        return {{ str(code.is_signed).lower() }};
{% endfor %}
    case PcmCode_Max:
        break;
    }
    return false;
}

// Code and endian to string
inline const char* pcm_to_str(PcmCode code, PcmEndian endian) {
    switch (code) {
{% for code in CODES %}
    case PcmCode_{{ code.code }}:
        switch (endian) {
{% for endian in ENDIANS %}
        case PcmEndian_{{ endian }}:
            return "{{ short_name(code, endian) }}";
{% endfor %}
        case PcmEndian_Max:
            break;
        }
        break;
{% endfor %}
    case PcmCode_Max:
        break;
    }
    return NULL;
}

// Code and endian from string
inline bool pcm_from_str(const char* str, PcmCode& code, PcmEndian& endian) {
{% for c0 in nth_chars(CODES) %}
    if (str[0] == '{{ c0 }}') {
{% for c1 in nth_chars(CODES, (c0,)) %}
        if (str[1] == '{{ c1 }}') {
{% for c2 in nth_chars(CODES, (c0, c1)) %}
            if (str[2] == '{{ c2 }}') {
{% for code in CODES %}
{% if tuple(code.short_name[:3]) == (c0, c1, c2) %}
{% for endian in ENDIANS %}
                if (strcmp(str, "{{ short_name(code, endian) }}") == 0) {
                    code = PcmCode_{{ code.code }};
                    endian = PcmEndian_{{ endian }};
                    return true;
                }
{% endfor %}
                return false;
{% endif %}
{% endfor %}
            }
{% endfor %}
{% for code in CODES %}
{% if tuple(code.short_name) == (c0, c1) %}
{% for endian in ENDIANS %}
            if (strcmp(str, "{{ short_name(code, endian) }}") == 0) {
                code = PcmCode_{{ code.code }};
                endian = PcmEndian_{{ endian }};
                return true;
            }
{% endfor %}
{% endif %}
{% endfor %}
            return false;
        }
{% endfor %}
        return false;
    }
{% endfor %}
    return false;
}

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_FUNCS_H_
'''.strip())

text = template.render(
    **dict(list(globals().items()) + list(builtins.__dict__.items())),
    )

os.chdir(os.path.dirname(os.path.abspath(__file__)))

with open('pcm_funcs.h', 'w') as fp:
    print(text, file=fp)
