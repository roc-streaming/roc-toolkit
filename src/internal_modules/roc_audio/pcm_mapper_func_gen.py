#! /usr/bin/env python3
import builtins
import math
import sys

try:
    import jinja2
except ImportError:
    print('''
error: can't import python module "jinja2", install it using "pip3 install Jinja2"
'''.strip(), file=sys.stderr)
    exit(1)

def compute_minmax(enc):
    suffix = ''
    if not enc['is_signed']:
        suffix += 'u'
    if enc['width'] >= 32:
        suffix += 'l'
    if enc['width'] >= 64:
        suffix += 'l'

    if enc['is_signed']:
        power = pow(2, enc['width']-1) - 1

        min_val = f'-{power}{suffix} - 1'
        max_val = f'{power}{suffix}'
    else:
        power = pow(2, enc['width']) - 1

        min_val = f'0{suffix}'
        max_val = f'{power}{suffix}'

    return min_val, max_val

def compute_masks(enc):
    value_mask = hex(int('1' * enc['width'], 2))
    if not enc['is_signed']:
        value_mask += 'u'

    if enc['is_signed']:
        sign_mask = hex(int('1' + '0' * (enc['width']-1), 2))
    else:
        sign_mask = None

    if enc['is_signed'] and enc['unpacked_width'] > enc['width']:
        lsb_mask = hex(
            int(
                ('1' * (enc['unpacked_width'] - enc['width']) +
                '0' * enc['width']),
                2))
    else:
        lsb_mask = None

    return value_mask, sign_mask, lsb_mask

def compute_octets(enc):
    return (math.ceil(enc['width'] / 8),
            math.ceil(enc['packed_width'] / 8),
            math.ceil(enc['unpacked_width'] / 8))

encodings = [
    {
        'encoding': 'SInt8',
        'signed_encoding': 'SInt8',
        'type': 'int8_t',
        'signed_type': 'int8_t',
        'unsigned_type': 'uint8_t',
        'is_integer': True,
        'is_signed': True,
        'width': 8,
        'packed_width': 8,
        'unpacked_width': 8,
    },
    {
        'encoding': 'UInt8',
        'signed_encoding': 'SInt8',
        'type': 'uint8_t',
        'signed_type': 'int8_t',
        'unsigned_type': 'uint8_t',
        'is_integer': True,
        'is_signed': False,
        'width': 8,
        'packed_width': 8,
        'unpacked_width': 8,
    },
    {
        'encoding': 'SInt16',
        'signed_encoding': 'SInt16',
        'type': 'int16_t',
        'signed_type': 'int16_t',
        'unsigned_type': 'uint16_t',
        'is_integer': True,
        'is_signed': True,
        'width': 16,
        'packed_width': 16,
        'unpacked_width': 16,
    },
    {
        'encoding': 'UInt16',
        'signed_encoding': 'SInt16',
        'type': 'uint16_t',
        'signed_type': 'int16_t',
        'unsigned_type': 'uint16_t',
        'is_integer': True,
        'is_signed': False,
        'width': 16,
        'packed_width': 16,
        'unpacked_width': 16,
    },
    {
        'encoding': 'SInt18',
        'signed_encoding': 'SInt18',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 18,
        'packed_width': 18,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt18',
        'signed_encoding': 'SInt18',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 18,
        'packed_width': 18,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt18_3B',
        'signed_encoding': 'SInt18_3B',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 18,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt18_3B',
        'signed_encoding': 'SInt18_3B',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 18,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt18_4B',
        'signed_encoding': 'SInt18_4B',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 18,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt18_4B',
        'signed_encoding': 'SInt18_4B',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 18,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt20',
        'signed_encoding': 'SInt20',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 20,
        'packed_width': 20,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt20',
        'signed_encoding': 'SInt20',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 20,
        'packed_width': 20,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt20_3B',
        'signed_encoding': 'SInt20_3B',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 20,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt20_3B',
        'signed_encoding': 'SInt20_3B',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 20,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt20_4B',
        'signed_encoding': 'SInt20_4B',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 20,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt20_4B',
        'signed_encoding': 'SInt20_4B',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 20,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt24',
        'signed_encoding': 'SInt24',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 24,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt24',
        'signed_encoding': 'SInt24',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 24,
        'packed_width': 24,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt24_4B',
        'signed_encoding': 'SInt24_4B',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 24,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt24_4B',
        'signed_encoding': 'SInt24_4B',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 24,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt32',
        'signed_encoding': 'SInt32',
        'type': 'int32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': True,
        'width': 32,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'UInt32',
        'signed_encoding': 'SInt32',
        'type': 'uint32_t',
        'signed_type': 'int32_t',
        'unsigned_type': 'uint32_t',
        'is_integer': True,
        'is_signed': False,
        'width': 32,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'SInt64',
        'signed_encoding': 'SInt64',
        'type': 'int64_t',
        'signed_type': 'int64_t',
        'unsigned_type': 'uint64_t',
        'is_integer': True,
        'is_signed': True,
        'width': 64,
        'packed_width': 64,
        'unpacked_width': 64,
    },
    {
        'encoding': 'UInt64',
        'signed_encoding': 'SInt64',
        'type': 'uint64_t',
        'signed_type': 'int64_t',
        'unsigned_type': 'uint64_t',
        'is_integer': True,
        'is_signed': False,
        'width': 64,
        'packed_width': 64,
        'unpacked_width': 64,
    },
    {
        'encoding': 'Float32',
        'signed_encoding': 'Float32',
        'type': 'float',
        'signed_type': 'float',
        'is_integer': False,
        'is_signed': True,
        'width': 32,
        'packed_width': 32,
        'unpacked_width': 32,
    },
    {
        'encoding': 'Float64',
        'signed_encoding': 'Float64',
        'type': 'double',
        'signed_type': 'double',
        'is_integer': False,
        'is_signed': True,
        'width': 64,
        'packed_width': 64,
        'unpacked_width': 64,
    },
]

endians = [
    'Native',
    'Big',
    'Little',
]

types = [
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

for enc in encodings:
    enc['min'] = f"pcm_{enc['encoding'].lower()}_min"
    enc['max'] = f"pcm_{enc['encoding'].lower()}_max"

    enc['signed_min'] = f"pcm_{enc['signed_encoding'].lower()}_min"
    enc['signed_max'] = f"pcm_{enc['signed_encoding'].lower()}_max"

    enc['min_value'], enc['max_value'] = compute_minmax(enc)

    enc['value_mask'], enc['sign_mask'], enc['lsb_mask'] = \
      compute_masks(enc)

    enc['significant_octets'], enc['packed_octets'], enc['unpacked_octets'] = \
      compute_octets(enc)

env = jinja2.Environment(
    trim_blocks=True,
    lstrip_blocks=True,
    undefined = jinja2.StrictUndefined)

template = env.from_string('''
/*
 * THIS FILE IS AUTO-GENERATED USING `pcm_mapper_func_gen.py'
 */

#ifndef ROC_AUDIO_PCM_MAPPER_FUNC_H_
#define ROC_AUDIO_PCM_MAPPER_FUNC_H_

#include "roc_audio/pcm_format.h"
#include "roc_core/attributes.h"
#include "roc_core/cpu_traits.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

{% for enc in encodings %}
{% if enc.is_integer %}
// {{ enc.encoding }} value range
const {{ enc.type }} {{ enc.min }} = {{ enc.min_value }};
const {{ enc.type }} {{ enc.max }} = {{ enc.max_value }};

{% endif %}
{% endfor %}
// Convert between signed and unsigned samples
template <PcmEncoding> struct pcm_sign_converter;

{% for enc in encodings %}
{% if enc.is_integer %}
// Convert {{ enc.encoding }} from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_{{ enc.encoding }}> {
{% if enc.is_signed %}
    // {{ enc.encoding }} from unsigned value
    static inline {{ enc.signed_type }} from_unsigned({{ enc.unsigned_type }} arg) {
        if (arg < {{ enc.unsigned_type }}({{ enc.signed_max }}) + 1) {
            return {{ enc.signed_type }}(arg) - {{ enc.signed_max }} - 1;
        }
        return {{ enc.signed_type }}(arg - {{ enc.unsigned_type }}({{ enc.signed_max }}) - 1);
    }

    // {{ enc.encoding }} to unsigned value
    static inline {{ enc.unsigned_type }} to_unsigned({{ enc.signed_type }} arg) {
        if (arg >= 0) {
            return {{ enc.unsigned_type }}(arg) + {{ enc.signed_max }} + 1;
        }
        return {{ enc.unsigned_type }}(arg + {{ enc.signed_max }} + 1);
    }
{% else %}
    // {{ enc.encoding }} from signed value
    static inline {{ enc.unsigned_type }} from_signed({{ enc.signed_type }} arg) {
        if (arg >= 0) {
            return {{ enc.unsigned_type }}(arg) + {{ enc.signed_max }} + 1;
        }
        return {{ enc.unsigned_type }}(arg + {{ enc.signed_max }} + 1);
    }

    // {{ enc.encoding }} to signed value
    static inline {{ enc.signed_type }} to_signed({{ enc.unsigned_type }} arg) {
        if (arg >= {{ enc.unsigned_type }}({{ enc.signed_max }}) + 1) {
            return {{ enc.signed_type }}(arg \
- {{ enc.unsigned_type }}({{ enc.signed_max }}) - 1);
        }
        return {{ enc.signed_type }}(arg - {{ enc.unsigned_type }}({{ enc.signed_max }}) - 1);
    }
{% endif %}
};

{% endif %}
{% endfor %}
// Convert between unpacked encodings
template <PcmEncoding InEnc, PcmEncoding OutEnc> struct pcm_encoding_converter;

{% for out in encodings %}
{% for in in encodings %}
{% set both_unsigned = not in.is_signed and not out.is_signed %}
// Convert {{ in.encoding }} to {{ out.encoding }}
template <> struct pcm_encoding_converter<PcmEncoding_{{ in.encoding }}, \
PcmEncoding_{{ out.encoding }}> {
    static inline {{ out.type }} convert({{ in.type }} arg) {
{% if in.encoding == out.encoding %}
        return arg;
{% else %}
{% if not in.is_signed and not both_unsigned %}
        // convert to signed
        {{ in.signed_type }} in = \
pcm_sign_converter<PcmEncoding_{{ in.encoding }}>::to_signed(arg);
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
        return pcm_sign_converter<PcmEncoding_{{ out.encoding }}>::from_signed(out);
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
#if ROC_CPU_BIG_ENDIAN
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

{% for type, size in types %}
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
template <PcmEncoding, PcmEndian> struct pcm_packer;

{% for enc in encodings %}
{% for endian in ['Big', 'Little'] %}
// {{ enc.encoding }} {{ endian }}-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_{{ enc.encoding }}, PcmEndian_{{ endian }}> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, {{ enc.type }} arg) {
        // native-endian view of octets
        pcm_sample<{{ enc.type }}> p;
        p.value = arg;

{% if enc.width < enc.packed_width %}
        // zeroise padding bits
        p.value &= {{ enc.value_mask }};

{% endif %}
        // write in {{ endian.lower() }}-endian order
{% for n in range(enc.packed_octets) %}
{% if endian == 'Big' %}
{% set n = enc.packed_octets - n - 1 %}
{% endif %}
{% if enc.packed_width % 8 == 0 %}
        pcm_aligned_write(buffer, bit_offset, p.octets.octet{{ n }});
{% else %}
        pcm_unaligned_write(buffer, bit_offset, \
{{ enc.packed_width % 8 if n == enc.packed_octets-1 else 8 }}, p.octets.octet{{ n }});
{% endif %}
{% endfor %}
    }

    // Unpack next sample from buffer
    static inline {{ enc.type }} unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<{{ enc.type }}> p;

        // read in {{ endian.lower() }}-endian order
{% for n in range(enc.unpacked_octets) %}
{% if endian == 'Big' %}
{% set n = enc.unpacked_octets - n - 1 %}
{% endif %}
{% if n >= enc.packed_octets %}
        p.octets.octet{{ n }} = 0;
{% elif enc.packed_width % 8 == 0 %}
        p.octets.octet{{ n }} = pcm_aligned_read(buffer, bit_offset);
{% else %}
        p.octets.octet{{ n }} = pcm_unaligned_read(buffer, bit_offset, \
{{ enc.packed_width % 8 if n == enc.packed_octets-1 else 8 }});
{% endif %}
{% endfor %}

{% if enc.width < enc.packed_width %}
        // zeroise padding bits
        p.value &= {{ enc.value_mask }};

{% endif %}
{% if enc.is_signed and enc.width < enc.unpacked_width %}
        if (p.value & {{ enc.sign_mask }}) {
            // sign extension
            p.value |= {{ enc.lsb_mask }};
        }

{% endif %}
        return p.value;
    }
};

{% endfor %}
{% endfor %}
// Map encoding and endian of samples
template <PcmEncoding InEnc, PcmEncoding OutEnc, PcmEndian InEnd, PcmEndian OutEnd>
struct pcm_mapper {
    static inline void map(const uint8_t* in_data,
                           size_t& in_bit_off,
                           uint8_t* out_data,
                           size_t& out_bit_off,
                           size_t n_samples) {
        for (size_t n = 0; n < n_samples; n++) {
            pcm_packer<OutEnc, OutEnd>::pack(
                out_data, out_bit_off,
                pcm_encoding_converter<InEnc, OutEnc>::convert(
                    pcm_packer<InEnc, InEnd>::unpack(in_data, in_bit_off)));
        }
    }
};

// Sample mapping function
typedef void (*pcm_mapper_func_t)(
    const uint8_t* in_data,
    size_t& in_bit_off,
    uint8_t* out_data,
    size_t& out_bit_off,
    size_t n_samples);

// Select mapper function
template <PcmEncoding InEnc, PcmEncoding OutEnc, PcmEndian InEnd, PcmEndian OutEnd>
pcm_mapper_func_t pcm_mapper_func() {
    return &pcm_mapper<InEnc, OutEnc, InEnd, OutEnd>::map;
}

// Select mapper function
template <PcmEncoding InEnc, PcmEncoding OutEnc, PcmEndian InEnd>
pcm_mapper_func_t pcm_mapper_func(PcmEndian out_endian) {
    switch (out_endian) {
{% for e in endians %}
    case PcmEndian_{{ e }}:
{% if e == 'Native' %}
#if ROC_CPU_BIG_ENDIAN
        return pcm_mapper_func<InEnc, OutEnc, InEnd, PcmEndian_Big>();
#else
        return pcm_mapper_func<InEnc, OutEnc, InEnd, PcmEndian_Little>();
#endif
{% else %}
        return pcm_mapper_func<InEnc, OutEnc, InEnd, PcmEndian_{{ e }}>();
{% endif %}
{% endfor %}
    }
    return NULL;
}

// Select mapper function
template <PcmEncoding InEnc, PcmEncoding OutEnc>
pcm_mapper_func_t pcm_mapper_func(PcmEndian in_endian, PcmEndian out_endian) {
    switch (in_endian) {
{% for e in endians %}
    case PcmEndian_{{ e }}:
{% if e == 'Native' %}
#if ROC_CPU_BIG_ENDIAN
        return pcm_mapper_func<InEnc, OutEnc, PcmEndian_Big>(out_endian);
#else
        return pcm_mapper_func<InEnc, OutEnc, PcmEndian_Little>(out_endian);
#endif
{% else %}
        return pcm_mapper_func<InEnc, OutEnc, PcmEndian_{{ e }}>(out_endian);
{% endif %}
{% endfor %}
    }
    return NULL;
}

// Select mapper function
template <PcmEncoding InEnc>
inline pcm_mapper_func_t pcm_mapper_func(PcmEncoding out_encoding,
                                         PcmEndian in_endian,
                                         PcmEndian out_endian) {
    switch (out_encoding) {
{% for e in encodings %}
    case PcmEncoding_{{ e.encoding }}:
        return pcm_mapper_func<InEnc, PcmEncoding_{{ e.encoding }}>(in_endian, out_endian);
{% endfor %}
    }
    return NULL;
}

// Select mapper function
inline pcm_mapper_func_t pcm_mapper_func(PcmEncoding in_encoding,
                                         PcmEncoding out_encoding,
                                         PcmEndian in_endian,
                                         PcmEndian out_endian) {
    switch (in_encoding) {
{% for e in encodings %}
    case PcmEncoding_{{ e.encoding }}:
        return pcm_mapper_func<PcmEncoding_{{ e.encoding }}>(out_encoding, \
in_endian, out_endian);
{% endfor %}
    }
    return NULL;
}

// Get number of bits per sample in packed format
inline size_t pcm_sample_bits(PcmEncoding encoding) {
    switch (encoding) {
{% for e in encodings %}
    case PcmEncoding_{{ e.encoding }}:
        return {{ e.packed_width }};
{% endfor %}
    }
    return 0;
}

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_MAPPER_FUNC_H_
'''.strip())

text = template.render(
    **dict(list(globals().items()) + list(builtins.__dict__.items())),
    )

print(text)
