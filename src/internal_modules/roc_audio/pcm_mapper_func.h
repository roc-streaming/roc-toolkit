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

// SInt8 value range
const int8_t pcm_sint8_min = -127 - 1;
const int8_t pcm_sint8_max = 127;

// UInt8 value range
const uint8_t pcm_uint8_min = 0u;
const uint8_t pcm_uint8_max = 255u;

// SInt16 value range
const int16_t pcm_sint16_min = -32767 - 1;
const int16_t pcm_sint16_max = 32767;

// UInt16 value range
const uint16_t pcm_uint16_min = 0u;
const uint16_t pcm_uint16_max = 65535u;

// SInt18 value range
const int32_t pcm_sint18_min = -131071 - 1;
const int32_t pcm_sint18_max = 131071;

// UInt18 value range
const uint32_t pcm_uint18_min = 0u;
const uint32_t pcm_uint18_max = 262143u;

// SInt18_3B value range
const int32_t pcm_sint18_3b_min = -131071 - 1;
const int32_t pcm_sint18_3b_max = 131071;

// UInt18_3B value range
const uint32_t pcm_uint18_3b_min = 0u;
const uint32_t pcm_uint18_3b_max = 262143u;

// SInt18_4B value range
const int32_t pcm_sint18_4b_min = -131071 - 1;
const int32_t pcm_sint18_4b_max = 131071;

// UInt18_4B value range
const uint32_t pcm_uint18_4b_min = 0u;
const uint32_t pcm_uint18_4b_max = 262143u;

// SInt20 value range
const int32_t pcm_sint20_min = -524287 - 1;
const int32_t pcm_sint20_max = 524287;

// UInt20 value range
const uint32_t pcm_uint20_min = 0u;
const uint32_t pcm_uint20_max = 1048575u;

// SInt20_3B value range
const int32_t pcm_sint20_3b_min = -524287 - 1;
const int32_t pcm_sint20_3b_max = 524287;

// UInt20_3B value range
const uint32_t pcm_uint20_3b_min = 0u;
const uint32_t pcm_uint20_3b_max = 1048575u;

// SInt20_4B value range
const int32_t pcm_sint20_4b_min = -524287 - 1;
const int32_t pcm_sint20_4b_max = 524287;

// UInt20_4B value range
const uint32_t pcm_uint20_4b_min = 0u;
const uint32_t pcm_uint20_4b_max = 1048575u;

// SInt24 value range
const int32_t pcm_sint24_min = -8388607 - 1;
const int32_t pcm_sint24_max = 8388607;

// UInt24 value range
const uint32_t pcm_uint24_min = 0u;
const uint32_t pcm_uint24_max = 16777215u;

// SInt24_4B value range
const int32_t pcm_sint24_4b_min = -8388607 - 1;
const int32_t pcm_sint24_4b_max = 8388607;

// UInt24_4B value range
const uint32_t pcm_uint24_4b_min = 0u;
const uint32_t pcm_uint24_4b_max = 16777215u;

// SInt32 value range
const int32_t pcm_sint32_min = -2147483647l - 1;
const int32_t pcm_sint32_max = 2147483647l;

// UInt32 value range
const uint32_t pcm_uint32_min = 0ul;
const uint32_t pcm_uint32_max = 4294967295ul;

// SInt64 value range
const int64_t pcm_sint64_min = -9223372036854775807ll - 1;
const int64_t pcm_sint64_max = 9223372036854775807ll;

// UInt64 value range
const uint64_t pcm_uint64_min = 0ull;
const uint64_t pcm_uint64_max = 18446744073709551615ull;

// Convert between signed and unsigned samples
template <PcmEncoding> struct pcm_sign_converter;

// Convert SInt8 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt8> {
    // SInt8 from unsigned value
    static inline int8_t from_unsigned(uint8_t arg) {
        if (arg < uint8_t(pcm_sint8_max) + 1) {
            return int8_t(arg) - pcm_sint8_max - 1;
        }
        return int8_t(arg - uint8_t(pcm_sint8_max) - 1);
    }

    // SInt8 to unsigned value
    static inline uint8_t to_unsigned(int8_t arg) {
        if (arg >= 0) {
            return uint8_t(arg) + pcm_sint8_max + 1;
        }
        return uint8_t(arg + pcm_sint8_max + 1);
    }
};

// Convert UInt8 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt8> {
    // UInt8 from signed value
    static inline uint8_t from_signed(int8_t arg) {
        if (arg >= 0) {
            return uint8_t(arg) + pcm_sint8_max + 1;
        }
        return uint8_t(arg + pcm_sint8_max + 1);
    }

    // UInt8 to signed value
    static inline int8_t to_signed(uint8_t arg) {
        if (arg >= uint8_t(pcm_sint8_max) + 1) {
            return int8_t(arg - uint8_t(pcm_sint8_max) - 1);
        }
        return int8_t(arg) - pcm_sint8_max - 1;
    }
};

// Convert SInt16 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt16> {
    // SInt16 from unsigned value
    static inline int16_t from_unsigned(uint16_t arg) {
        if (arg < uint16_t(pcm_sint16_max) + 1) {
            return int16_t(arg) - pcm_sint16_max - 1;
        }
        return int16_t(arg - uint16_t(pcm_sint16_max) - 1);
    }

    // SInt16 to unsigned value
    static inline uint16_t to_unsigned(int16_t arg) {
        if (arg >= 0) {
            return uint16_t(arg) + pcm_sint16_max + 1;
        }
        return uint16_t(arg + pcm_sint16_max + 1);
    }
};

// Convert UInt16 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt16> {
    // UInt16 from signed value
    static inline uint16_t from_signed(int16_t arg) {
        if (arg >= 0) {
            return uint16_t(arg) + pcm_sint16_max + 1;
        }
        return uint16_t(arg + pcm_sint16_max + 1);
    }

    // UInt16 to signed value
    static inline int16_t to_signed(uint16_t arg) {
        if (arg >= uint16_t(pcm_sint16_max) + 1) {
            return int16_t(arg - uint16_t(pcm_sint16_max) - 1);
        }
        return int16_t(arg) - pcm_sint16_max - 1;
    }
};

// Convert SInt18 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt18> {
    // SInt18 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint18_max) + 1) {
            return int32_t(arg) - pcm_sint18_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint18_max) - 1);
    }

    // SInt18 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_max + 1;
        }
        return uint32_t(arg + pcm_sint18_max + 1);
    }
};

// Convert UInt18 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt18> {
    // UInt18 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_max + 1;
        }
        return uint32_t(arg + pcm_sint18_max + 1);
    }

    // UInt18 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint18_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint18_max) - 1);
        }
        return int32_t(arg) - pcm_sint18_max - 1;
    }
};

// Convert SInt18_3B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt18_3B> {
    // SInt18_3B from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint18_3b_max) + 1) {
            return int32_t(arg) - pcm_sint18_3b_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint18_3b_max) - 1);
    }

    // SInt18_3B to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_3b_max + 1;
        }
        return uint32_t(arg + pcm_sint18_3b_max + 1);
    }
};

// Convert UInt18_3B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt18_3B> {
    // UInt18_3B from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_3b_max + 1;
        }
        return uint32_t(arg + pcm_sint18_3b_max + 1);
    }

    // UInt18_3B to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint18_3b_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint18_3b_max) - 1);
        }
        return int32_t(arg) - pcm_sint18_3b_max - 1;
    }
};

// Convert SInt18_4B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt18_4B> {
    // SInt18_4B from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint18_4b_max) + 1) {
            return int32_t(arg) - pcm_sint18_4b_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint18_4b_max) - 1);
    }

    // SInt18_4B to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_4b_max + 1;
        }
        return uint32_t(arg + pcm_sint18_4b_max + 1);
    }
};

// Convert UInt18_4B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt18_4B> {
    // UInt18_4B from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_4b_max + 1;
        }
        return uint32_t(arg + pcm_sint18_4b_max + 1);
    }

    // UInt18_4B to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint18_4b_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint18_4b_max) - 1);
        }
        return int32_t(arg) - pcm_sint18_4b_max - 1;
    }
};

// Convert SInt20 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt20> {
    // SInt20 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint20_max) + 1) {
            return int32_t(arg) - pcm_sint20_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint20_max) - 1);
    }

    // SInt20 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_max + 1;
        }
        return uint32_t(arg + pcm_sint20_max + 1);
    }
};

// Convert UInt20 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt20> {
    // UInt20 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_max + 1;
        }
        return uint32_t(arg + pcm_sint20_max + 1);
    }

    // UInt20 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint20_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint20_max) - 1);
        }
        return int32_t(arg) - pcm_sint20_max - 1;
    }
};

// Convert SInt20_3B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt20_3B> {
    // SInt20_3B from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint20_3b_max) + 1) {
            return int32_t(arg) - pcm_sint20_3b_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint20_3b_max) - 1);
    }

    // SInt20_3B to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_3b_max + 1;
        }
        return uint32_t(arg + pcm_sint20_3b_max + 1);
    }
};

// Convert UInt20_3B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt20_3B> {
    // UInt20_3B from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_3b_max + 1;
        }
        return uint32_t(arg + pcm_sint20_3b_max + 1);
    }

    // UInt20_3B to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint20_3b_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint20_3b_max) - 1);
        }
        return int32_t(arg) - pcm_sint20_3b_max - 1;
    }
};

// Convert SInt20_4B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt20_4B> {
    // SInt20_4B from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint20_4b_max) + 1) {
            return int32_t(arg) - pcm_sint20_4b_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint20_4b_max) - 1);
    }

    // SInt20_4B to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_4b_max + 1;
        }
        return uint32_t(arg + pcm_sint20_4b_max + 1);
    }
};

// Convert UInt20_4B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt20_4B> {
    // UInt20_4B from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_4b_max + 1;
        }
        return uint32_t(arg + pcm_sint20_4b_max + 1);
    }

    // UInt20_4B to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint20_4b_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint20_4b_max) - 1);
        }
        return int32_t(arg) - pcm_sint20_4b_max - 1;
    }
};

// Convert SInt24 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt24> {
    // SInt24 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint24_max) + 1) {
            return int32_t(arg) - pcm_sint24_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint24_max) - 1);
    }

    // SInt24 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint24_max + 1;
        }
        return uint32_t(arg + pcm_sint24_max + 1);
    }
};

// Convert UInt24 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt24> {
    // UInt24 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint24_max + 1;
        }
        return uint32_t(arg + pcm_sint24_max + 1);
    }

    // UInt24 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint24_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint24_max) - 1);
        }
        return int32_t(arg) - pcm_sint24_max - 1;
    }
};

// Convert SInt24_4B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt24_4B> {
    // SInt24_4B from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint24_4b_max) + 1) {
            return int32_t(arg) - pcm_sint24_4b_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint24_4b_max) - 1);
    }

    // SInt24_4B to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint24_4b_max + 1;
        }
        return uint32_t(arg + pcm_sint24_4b_max + 1);
    }
};

// Convert UInt24_4B from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt24_4B> {
    // UInt24_4B from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint24_4b_max + 1;
        }
        return uint32_t(arg + pcm_sint24_4b_max + 1);
    }

    // UInt24_4B to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint24_4b_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint24_4b_max) - 1);
        }
        return int32_t(arg) - pcm_sint24_4b_max - 1;
    }
};

// Convert SInt32 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt32> {
    // SInt32 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint32_max) + 1) {
            return int32_t(arg) - pcm_sint32_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint32_max) - 1);
    }

    // SInt32 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint32_max + 1;
        }
        return uint32_t(arg + pcm_sint32_max + 1);
    }
};

// Convert UInt32 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt32> {
    // UInt32 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint32_max + 1;
        }
        return uint32_t(arg + pcm_sint32_max + 1);
    }

    // UInt32 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint32_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint32_max) - 1);
        }
        return int32_t(arg) - pcm_sint32_max - 1;
    }
};

// Convert SInt64 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_SInt64> {
    // SInt64 from unsigned value
    static inline int64_t from_unsigned(uint64_t arg) {
        if (arg < uint64_t(pcm_sint64_max) + 1) {
            return int64_t(arg) - pcm_sint64_max - 1;
        }
        return int64_t(arg - uint64_t(pcm_sint64_max) - 1);
    }

    // SInt64 to unsigned value
    static inline uint64_t to_unsigned(int64_t arg) {
        if (arg >= 0) {
            return uint64_t(arg) + pcm_sint64_max + 1;
        }
        return uint64_t(arg + pcm_sint64_max + 1);
    }
};

// Convert UInt64 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmEncoding_UInt64> {
    // UInt64 from signed value
    static inline uint64_t from_signed(int64_t arg) {
        if (arg >= 0) {
            return uint64_t(arg) + pcm_sint64_max + 1;
        }
        return uint64_t(arg + pcm_sint64_max + 1);
    }

    // UInt64 to signed value
    static inline int64_t to_signed(uint64_t arg) {
        if (arg >= uint64_t(pcm_sint64_max) + 1) {
            return int64_t(arg - uint64_t(pcm_sint64_max) - 1);
        }
        return int64_t(arg) - pcm_sint64_max - 1;
    }
};

// Convert between unpacked encodings
template <PcmEncoding InEnc, PcmEncoding OutEnc> struct pcm_encoding_converter;

// Convert SInt8 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt8> {
    static inline int8_t convert(int8_t arg) {
        return arg;
    }
};

// Convert UInt8 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt8> {
    static inline int8_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int8_t out;
        out = in;

        return out;
    }
};

// Convert SInt16 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt8> {
    static inline int8_t convert(int16_t arg) {
        int16_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int16_t(pcm_sint16_max - (int16_t(1) << 7))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint16_t(in + (int16_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert UInt16 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt8> {
    static inline int8_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int16_t(pcm_sint16_max - (int16_t(1) << 7))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint16_t(in + (int16_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert SInt18 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert UInt18 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert SInt18_3B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3b_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert UInt18_3B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3b_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert SInt18_4B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4b_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert UInt18_4B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4b_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert SInt20 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt20 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt20_3B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt20_3B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt20_4B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt20_4B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt24 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert UInt24 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert SInt32 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 23))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 23)) >> 24);
        }

        return out;
    }
};

// Convert UInt32 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 23))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 23)) >> 24);
        }

        return out;
    }
};

// Convert SInt64 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt8> {
    static inline int8_t convert(int64_t arg) {
        int64_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 55))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint64_t(in + (int64_t(1) << 55)) >> 56);
        }

        return out;
    }
};

// Convert UInt64 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt8> {
    static inline int8_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 55))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint64_t(in + (int64_t(1) << 55)) >> 56);
        }

        return out;
    }
};

// Convert Float32 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt8> {
    static inline int8_t convert(float arg) {
        float in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * (pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt8
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt8> {
    static inline int8_t convert(double arg) {
        double in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * (pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt8> {
    static inline uint8_t convert(int8_t arg) {
        int8_t in = arg;

        int8_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt8 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint8_t arg) {
        return arg;
    }
};

// Convert SInt16 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt8> {
    static inline uint8_t convert(int16_t arg) {
        int16_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int16_t(pcm_sint16_max - (int16_t(1) << 7))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint16_t(in + (int16_t(1) << 7)) >> 8);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt16 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 8);

        return out;
    }
};

// Convert SInt18 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt18 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 10);

        return out;
    }
};

// Convert SInt18_3B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3b_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 10);

        return out;
    }
};

// Convert SInt18_4B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4b_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 10);

        return out;
    }
};

// Convert SInt20 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt20 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 12);

        return out;
    }
};

// Convert SInt20_3B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 12);

        return out;
    }
};

// Convert SInt20_4B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 12);

        return out;
    }
};

// Convert SInt24 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt24 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 16);

        return out;
    }
};

// Convert SInt24_4B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 16);

        return out;
    }
};

// Convert SInt32 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 23))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 23)) >> 24);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt32 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 24);

        return out;
    }
};

// Convert SInt64 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt8> {
    static inline uint8_t convert(int64_t arg) {
        int64_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 55))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint64_t(in + (int64_t(1) << 55)) >> 56);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert UInt64 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt8> {
    static inline uint8_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 56);

        return out;
    }
};

// Convert Float32 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt8> {
    static inline uint8_t convert(float arg) {
        float in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * (pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert Float64 to UInt8
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt8> {
    static inline uint8_t convert(double arg) {
        double in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * (pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt8>::from_signed(out);
    }
};

// Convert SInt8 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt16> {
    static inline int16_t convert(int8_t arg) {
        int8_t in = arg;

        int16_t out;
        // upscale signed integer
        out = int16_t(uint16_t(in) << 8);

        return out;
    }
};

// Convert UInt8 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt16> {
    static inline int16_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int16_t out;
        // upscale signed integer
        out = int16_t(uint16_t(in) << 8);

        return out;
    }
};

// Convert SInt16 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt16> {
    static inline int16_t convert(int16_t arg) {
        return arg;
    }
};

// Convert UInt16 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt16> {
    static inline int16_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int16_t out;
        out = in;

        return out;
    }
};

// Convert SInt18 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt18 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt18_3B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt18_3B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt18_4B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt18_4B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt20 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt20_3B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt20_3B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt20_4B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt20_4B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt24 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert UInt24 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert SInt32 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert UInt32 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert SInt64 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt16> {
    static inline int16_t convert(int64_t arg) {
        int64_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 47))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint64_t(in + (int64_t(1) << 47)) >> 48);
        }

        return out;
    }
};

// Convert UInt64 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt16> {
    static inline int16_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 47))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint64_t(in + (int64_t(1) << 47)) >> 48);
        }

        return out;
    }
};

// Convert Float32 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt16> {
    static inline int16_t convert(float arg) {
        float in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * (pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt16
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt16> {
    static inline int16_t convert(double arg) {
        double in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * (pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt16> {
    static inline uint16_t convert(int8_t arg) {
        int8_t in = arg;

        int16_t out;
        // upscale signed integer
        out = int16_t(uint16_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt8 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint16_t out;
        // upscale unsigned integer
        out = uint16_t(uint16_t(in) << 8);

        return out;
    }
};

// Convert SInt16 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt16> {
    static inline uint16_t convert(int16_t arg) {
        int16_t in = arg;

        int16_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt16 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint16_t arg) {
        return arg;
    }
};

// Convert SInt18 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt18 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 2);

        return out;
    }
};

// Convert SInt18_3B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 2);

        return out;
    }
};

// Convert SInt18_4B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 2);

        return out;
    }
};

// Convert SInt20 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt20 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 4);

        return out;
    }
};

// Convert SInt20_3B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 4);

        return out;
    }
};

// Convert SInt20_4B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 4);

        return out;
    }
};

// Convert SInt24 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt24 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 8);

        return out;
    }
};

// Convert SInt24_4B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 8);

        return out;
    }
};

// Convert SInt32 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt32 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 16);

        return out;
    }
};

// Convert SInt64 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt16> {
    static inline uint16_t convert(int64_t arg) {
        int64_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 47))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint64_t(in + (int64_t(1) << 47)) >> 48);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert UInt64 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt16> {
    static inline uint16_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 48);

        return out;
    }
};

// Convert Float32 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt16> {
    static inline uint16_t convert(float arg) {
        float in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * (pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert Float64 to UInt16
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt16> {
    static inline uint16_t convert(double arg) {
        double in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * (pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt16>::from_signed(out);
    }
};

// Convert SInt8 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt18> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert UInt8 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt18> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt18> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt16 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt18> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt18 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_3B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_4B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_3B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_3B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_4B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_4B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt24 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt32 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert UInt32 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert SInt64 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt18> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert UInt64 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt18> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert Float32 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt18> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt18
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt18> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt18> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt8 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt18> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt16 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt18 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt18_3B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt20 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_3B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_4B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt24 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt24 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt24_4B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt32 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt32 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 14);

        return out;
    }
};

// Convert SInt64 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt18> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert UInt64 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt18> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 46);

        return out;
    }
};

// Convert Float32 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt18> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert Float64 to UInt18
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt18> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18>::from_signed(out);
    }
};

// Convert SInt8 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert UInt8 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt16 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt18_3B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_4B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_3B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_3B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_4B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_4B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt24 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt32 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert UInt32 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert SInt64 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert UInt64 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert Float32 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_3b_max + 1.0);
        if (d < pcm_sint18_3b_min) {
            // clip
            out = pcm_sint18_3b_min;
        } else if (d >= pcm_sint18_3b_max + 1.0) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt18_3B> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_3b_max + 1.0);
        if (d < pcm_sint18_3b_min) {
            // clip
            out = pcm_sint18_3b_min;
        } else if (d >= pcm_sint18_3b_max + 1.0) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt8 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt16 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt18 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt18_4B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt20 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_3B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_4B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt24 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt24 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt24_4B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt32 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt32 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 14);

        return out;
    }
};

// Convert SInt64 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert UInt64 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 46);

        return out;
    }
};

// Convert Float32 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_3b_max + 1.0);
        if (d < pcm_sint18_3b_min) {
            // clip
            out = pcm_sint18_3b_min;
        } else if (d >= pcm_sint18_3b_max + 1.0) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert Float64 to UInt18_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt18_3B> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_3b_max + 1.0);
        if (d < pcm_sint18_3b_min) {
            // clip
            out = pcm_sint18_3b_min;
        } else if (d >= pcm_sint18_3b_max + 1.0) {
            // clip
            out = pcm_sint18_3b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_3B>::from_signed(out);
    }
};

// Convert SInt8 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert UInt8 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt16 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_3B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt18_4B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_3B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_3B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_4B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_4B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt24 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt32 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert UInt32 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert SInt64 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert UInt64 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert Float32 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_4b_max + 1.0);
        if (d < pcm_sint18_4b_min) {
            // clip
            out = pcm_sint18_4b_min;
        } else if (d >= pcm_sint18_4b_max + 1.0) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt18_4B> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_4b_max + 1.0);
        if (d < pcm_sint18_4b_min) {
            // clip
            out = pcm_sint18_4b_min;
        } else if (d >= pcm_sint18_4b_max + 1.0) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt8 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt16 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt18 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt20 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt20 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_3B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_4B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4b_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt24 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt24 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt24_4B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt32 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt32 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 14);

        return out;
    }
};

// Convert SInt64 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert UInt64 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 46);

        return out;
    }
};

// Convert Float32 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_4b_max + 1.0);
        if (d < pcm_sint18_4b_min) {
            // clip
            out = pcm_sint18_4b_min;
        } else if (d >= pcm_sint18_4b_max + 1.0) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert Float64 to UInt18_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt18_4B> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint18_4b_max + 1.0);
        if (d < pcm_sint18_4b_min) {
            // clip
            out = pcm_sint18_4b_min;
        } else if (d >= pcm_sint18_4b_max + 1.0) {
            // clip
            out = pcm_sint18_4b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt18_4B>::from_signed(out);
    }
};

// Convert SInt8 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt20> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt8 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt20> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt20> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt16 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt20> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_3B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_4B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt20 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_3B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_4B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt32 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt32 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt64 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt20> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert UInt64 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt20> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert Float32 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt20> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt20
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt20> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt20> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt8 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt20> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt16 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt18 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt20 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt20_3B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt24 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt24_4B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt32 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt32 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 12);

        return out;
    }
};

// Convert SInt64 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt20> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert UInt64 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt20> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 44);

        return out;
    }
};

// Convert Float32 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt20> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert Float64 to UInt20
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt20> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20>::from_signed(out);
    }
};

// Convert SInt8 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt8 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt16 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_3B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_4B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt20_3B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_4B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt32 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt32 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt64 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert UInt64 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert Float32 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_3b_max + 1.0);
        if (d < pcm_sint20_3b_min) {
            // clip
            out = pcm_sint20_3b_min;
        } else if (d >= pcm_sint20_3b_max + 1.0) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt20_3B> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_3b_max + 1.0);
        if (d < pcm_sint20_3b_min) {
            // clip
            out = pcm_sint20_3b_min;
        } else if (d >= pcm_sint20_3b_max + 1.0) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt8 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt16 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt18 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt20 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt20_4B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt24 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt24_4B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt32 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt32 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 12);

        return out;
    }
};

// Convert SInt64 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert UInt64 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 44);

        return out;
    }
};

// Convert Float32 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_3b_max + 1.0);
        if (d < pcm_sint20_3b_min) {
            // clip
            out = pcm_sint20_3b_min;
        } else if (d >= pcm_sint20_3b_max + 1.0) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert Float64 to UInt20_3B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt20_3B> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_3b_max + 1.0);
        if (d < pcm_sint20_3b_min) {
            // clip
            out = pcm_sint20_3b_min;
        } else if (d >= pcm_sint20_3b_max + 1.0) {
            // clip
            out = pcm_sint20_3b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_3B>::from_signed(out);
    }
};

// Convert SInt8 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt8 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt16 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_3B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_4B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_3B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt20_4B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt24_4B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24_4B to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt32 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt32 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt64 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert UInt64 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert Float32 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_4b_max + 1.0);
        if (d < pcm_sint20_4b_min) {
            // clip
            out = pcm_sint20_4b_min;
        } else if (d >= pcm_sint20_4b_max + 1.0) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt20_4B> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_4b_max + 1.0);
        if (d < pcm_sint20_4b_min) {
            // clip
            out = pcm_sint20_4b_min;
        } else if (d >= pcm_sint20_4b_max + 1.0) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt8 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt16 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt18 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt20 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt24 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt24 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt24_4B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4b_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt32 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt32 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 12);

        return out;
    }
};

// Convert SInt64 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert UInt64 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 44);

        return out;
    }
};

// Convert Float32 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_4b_max + 1.0);
        if (d < pcm_sint20_4b_min) {
            // clip
            out = pcm_sint20_4b_min;
        } else if (d >= pcm_sint20_4b_max + 1.0) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert Float64 to UInt20_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt20_4B> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint20_4b_max + 1.0);
        if (d < pcm_sint20_4b_min) {
            // clip
            out = pcm_sint20_4b_min;
        } else if (d >= pcm_sint20_4b_max + 1.0) {
            // clip
            out = pcm_sint20_4b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt20_4B>::from_signed(out);
    }
};

// Convert SInt8 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt24> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert UInt8 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt24> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt24> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt16 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt24> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_3B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_4B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_3B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_4B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt24 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24_4B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt24_4B to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt32 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert UInt32 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert SInt64 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt24> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        return out;
    }
};

// Convert UInt64 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt24> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        return out;
    }
};

// Convert Float32 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt24> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt24
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt24> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt24> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt8 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt24> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt16 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt18 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt20 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt24 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt24_4B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt32 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt32 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 8);

        return out;
    }
};

// Convert SInt64 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt24> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert UInt64 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt24> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 40);

        return out;
    }
};

// Convert Float32 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt24> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert Float64 to UInt24
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt24> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24>::from_signed(out);
    }
};

// Convert SInt8 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert UInt8 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt16 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_3B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_4B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_3B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_4B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt24 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24_4B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt24_4B to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt32 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert UInt32 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert SInt64 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        return out;
    }
};

// Convert UInt64 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        return out;
    }
};

// Convert Float32 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_4b_max + 1.0);
        if (d < pcm_sint24_4b_min) {
            // clip
            out = pcm_sint24_4b_min;
        } else if (d >= pcm_sint24_4b_max + 1.0) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt24_4B> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_4b_max + 1.0);
        if (d < pcm_sint24_4b_min) {
            // clip
            out = pcm_sint24_4b_min;
        } else if (d >= pcm_sint24_4b_max + 1.0) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt8 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt16 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt18 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt20 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt24 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24_4B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt32 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt32 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 8);

        return out;
    }
};

// Convert SInt64 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert UInt64 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 40);

        return out;
    }
};

// Convert Float32 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_4b_max + 1.0);
        if (d < pcm_sint24_4b_min) {
            // clip
            out = pcm_sint24_4b_min;
        } else if (d >= pcm_sint24_4b_max + 1.0) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert Float64 to UInt24_4B
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt24_4B> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint24_4b_max + 1.0);
        if (d < pcm_sint24_4b_min) {
            // clip
            out = pcm_sint24_4b_min;
        } else if (d >= pcm_sint24_4b_max + 1.0) {
            // clip
            out = pcm_sint24_4b_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt24_4B>::from_signed(out);
    }
};

// Convert SInt8 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt32> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 24);

        return out;
    }
};

// Convert UInt8 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt32> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 24);

        return out;
    }
};

// Convert SInt16 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt32> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert UInt16 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt32> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt18 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert UInt18 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_3B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert UInt18_3B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_4B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert UInt18_4B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt20 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt20 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_3B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt20_3B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_4B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt20_4B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt24 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt24 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt24_4B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt24_4B to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt32 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt32> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt32 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt64 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt32> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 31))) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 31)) >> 32);
        }

        return out;
    }
};

// Convert UInt64 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt32> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 31))) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 31)) >> 32);
        }

        return out;
    }
};

// Convert Float32 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt32> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt32
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt32> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt32> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 24);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt8 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 24);

        return out;
    }
};

// Convert SInt16 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt32> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt16 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt18 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt18 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_3B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_4B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt20 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt20 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_3B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_4B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt24 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt24 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt24_4B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt32 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt32 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt64 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt32> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 31))) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 31)) >> 32);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert UInt64 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt32> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 32);

        return out;
    }
};

// Convert Float32 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt32> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert Float64 to UInt32
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt32> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * (pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt32>::from_signed(out);
    }
};

// Convert SInt8 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_SInt64> {
    static inline int64_t convert(int8_t arg) {
        int8_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 56);

        return out;
    }
};

// Convert UInt8 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_SInt64> {
    static inline int64_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 56);

        return out;
    }
};

// Convert SInt16 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_SInt64> {
    static inline int64_t convert(int16_t arg) {
        int16_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 48);

        return out;
    }
};

// Convert UInt16 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_SInt64> {
    static inline int64_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 48);

        return out;
    }
};

// Convert SInt18 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert UInt18 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_3B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert UInt18_3B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_4B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert UInt18_4B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt20 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert UInt20 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_3B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert UInt20_3B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_4B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert UInt20_4B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt24 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert UInt24 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt24_4B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert UInt24_4B to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt32 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 32);

        return out;
    }
};

// Convert UInt32 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 32);

        return out;
    }
};

// Convert SInt64 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_SInt64> {
    static inline int64_t convert(int64_t arg) {
        return arg;
    }
};

// Convert UInt64 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_SInt64> {
    static inline int64_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        int64_t out;
        out = in;

        return out;
    }
};

// Convert Float32 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_SInt64> {
    static inline int64_t convert(float arg) {
        float in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * (pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt64
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_SInt64> {
    static inline int64_t convert(double arg) {
        double in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * (pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_UInt64> {
    static inline uint64_t convert(int8_t arg) {
        int8_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 56);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt8 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 56);

        return out;
    }
};

// Convert SInt16 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_UInt64> {
    static inline uint64_t convert(int16_t arg) {
        int16_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 48);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt16 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 48);

        return out;
    }
};

// Convert SInt18 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt18 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_3B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt18_3B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_4B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt18_4B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt20 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt20 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_3B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt20_3B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_4B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt20_4B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt24 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt24 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt24_4B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt24_4B to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt32 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 32);

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt32 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 32);

        return out;
    }
};

// Convert SInt64 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_UInt64> {
    static inline uint64_t convert(int64_t arg) {
        int64_t in = arg;

        int64_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert UInt64 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_UInt64> {
    static inline uint64_t convert(uint64_t arg) {
        return arg;
    }
};

// Convert Float32 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_UInt64> {
    static inline uint64_t convert(float arg) {
        float in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * (pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert Float64 to UInt64
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_UInt64> {
    static inline uint64_t convert(double arg) {
        double in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * (pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmEncoding_UInt64>::from_signed(out);
    }
};

// Convert SInt8 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_Float32> {
    static inline float convert(int8_t arg) {
        int8_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert UInt8 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_Float32> {
    static inline float convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert SInt16 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_Float32> {
    static inline float convert(int16_t arg) {
        int16_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert UInt16 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_Float32> {
    static inline float convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert SInt18 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert UInt18 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert SInt18_3B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint18_3b_max + 1.0)));

        return out;
    }
};

// Convert UInt18_3B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint18_3b_max + 1.0)));

        return out;
    }
};

// Convert SInt18_4B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint18_4b_max + 1.0)));

        return out;
    }
};

// Convert UInt18_4B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint18_4b_max + 1.0)));

        return out;
    }
};

// Convert SInt20 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert UInt20 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert SInt20_3B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint20_3b_max + 1.0)));

        return out;
    }
};

// Convert UInt20_3B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint20_3b_max + 1.0)));

        return out;
    }
};

// Convert SInt20_4B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint20_4b_max + 1.0)));

        return out;
    }
};

// Convert UInt20_4B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint20_4b_max + 1.0)));

        return out;
    }
};

// Convert SInt24 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert UInt24 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert SInt24_4B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint24_4b_max + 1.0)));

        return out;
    }
};

// Convert UInt24_4B to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint24_4b_max + 1.0)));

        return out;
    }
};

// Convert SInt32 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert UInt32 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert SInt64 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_Float32> {
    static inline float convert(int64_t arg) {
        int64_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert UInt64 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_Float32> {
    static inline float convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / (pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert Float32 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_Float32> {
    static inline float convert(float arg) {
        return arg;
    }
};

// Convert Float64 to Float32
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_Float32> {
    static inline float convert(double arg) {
        double in = arg;

        float out;
        // float to float
        out = float(in);

        return out;
    }
};

// Convert SInt8 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt8, PcmEncoding_Float64> {
    static inline double convert(int8_t arg) {
        int8_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert UInt8 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt8, PcmEncoding_Float64> {
    static inline double convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmEncoding_UInt8>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert SInt16 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt16, PcmEncoding_Float64> {
    static inline double convert(int16_t arg) {
        int16_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert UInt16 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt16, PcmEncoding_Float64> {
    static inline double convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmEncoding_UInt16>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert SInt18 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert UInt18 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert SInt18_3B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_3B, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint18_3b_max + 1.0)));

        return out;
    }
};

// Convert UInt18_3B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_3B, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_3B>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint18_3b_max + 1.0)));

        return out;
    }
};

// Convert SInt18_4B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt18_4B, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint18_4b_max + 1.0)));

        return out;
    }
};

// Convert UInt18_4B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt18_4B, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt18_4B>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint18_4b_max + 1.0)));

        return out;
    }
};

// Convert SInt20 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert UInt20 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert SInt20_3B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_3B, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint20_3b_max + 1.0)));

        return out;
    }
};

// Convert UInt20_3B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_3B, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_3B>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint20_3b_max + 1.0)));

        return out;
    }
};

// Convert SInt20_4B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt20_4B, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint20_4b_max + 1.0)));

        return out;
    }
};

// Convert UInt20_4B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt20_4B, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt20_4B>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint20_4b_max + 1.0)));

        return out;
    }
};

// Convert SInt24 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt24, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert UInt24 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt24, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert SInt24_4B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt24_4B, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint24_4b_max + 1.0)));

        return out;
    }
};

// Convert UInt24_4B to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt24_4B, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt24_4B>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint24_4b_max + 1.0)));

        return out;
    }
};

// Convert SInt32 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt32, PcmEncoding_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert UInt32 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt32, PcmEncoding_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmEncoding_UInt32>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert SInt64 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_SInt64, PcmEncoding_Float64> {
    static inline double convert(int64_t arg) {
        int64_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert UInt64 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_UInt64, PcmEncoding_Float64> {
    static inline double convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmEncoding_UInt64>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / (pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert Float32 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_Float32, PcmEncoding_Float64> {
    static inline double convert(float arg) {
        float in = arg;

        double out;
        // float to float
        out = double(in);

        return out;
    }
};

// Convert Float64 to Float64
template <> struct pcm_encoding_converter<PcmEncoding_Float64, PcmEncoding_Float64> {
    static inline double convert(double arg) {
        return arg;
    }
};

// N-byte native-endian packed octet array
template <size_t N> struct pcm_octets;

// 1-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<1> {
#if ROC_CPU_BIG_ENDIAN
    uint8_t octet0;
#else
    uint8_t octet0;
#endif
} ROC_ATTR_PACKED_END;

// 2-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<2> {
#if ROC_CPU_BIG_ENDIAN
    uint8_t octet1;
    uint8_t octet0;
#else
    uint8_t octet0;
    uint8_t octet1;
#endif
} ROC_ATTR_PACKED_END;

// 4-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<4> {
#if ROC_CPU_BIG_ENDIAN
    uint8_t octet3;
    uint8_t octet2;
    uint8_t octet1;
    uint8_t octet0;
#else
    uint8_t octet0;
    uint8_t octet1;
    uint8_t octet2;
    uint8_t octet3;
#endif
} ROC_ATTR_PACKED_END;

// 8-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<8> {
#if ROC_CPU_BIG_ENDIAN
    uint8_t octet7;
    uint8_t octet6;
    uint8_t octet5;
    uint8_t octet4;
    uint8_t octet3;
    uint8_t octet2;
    uint8_t octet1;
    uint8_t octet0;
#else
    uint8_t octet0;
    uint8_t octet1;
    uint8_t octet2;
    uint8_t octet3;
    uint8_t octet4;
    uint8_t octet5;
    uint8_t octet6;
    uint8_t octet7;
#endif
} ROC_ATTR_PACKED_END;

// N-byte native-endian sample
template <class T> struct pcm_sample;

// int8_t native-endian sample
template <> struct pcm_sample<int8_t> {
    union {
        int8_t value;
        pcm_octets<1> octets;
    };
};

// uint8_t native-endian sample
template <> struct pcm_sample<uint8_t> {
    union {
        uint8_t value;
        pcm_octets<1> octets;
    };
};

// int16_t native-endian sample
template <> struct pcm_sample<int16_t> {
    union {
        int16_t value;
        pcm_octets<2> octets;
    };
};

// uint16_t native-endian sample
template <> struct pcm_sample<uint16_t> {
    union {
        uint16_t value;
        pcm_octets<2> octets;
    };
};

// int32_t native-endian sample
template <> struct pcm_sample<int32_t> {
    union {
        int32_t value;
        pcm_octets<4> octets;
    };
};

// uint32_t native-endian sample
template <> struct pcm_sample<uint32_t> {
    union {
        uint32_t value;
        pcm_octets<4> octets;
    };
};

// int64_t native-endian sample
template <> struct pcm_sample<int64_t> {
    union {
        int64_t value;
        pcm_octets<8> octets;
    };
};

// uint64_t native-endian sample
template <> struct pcm_sample<uint64_t> {
    union {
        uint64_t value;
        pcm_octets<8> octets;
    };
};

// float native-endian sample
template <> struct pcm_sample<float> {
    union {
        float value;
        pcm_octets<4> octets;
    };
};

// double native-endian sample
template <> struct pcm_sample<double> {
    union {
        double value;
        pcm_octets<8> octets;
    };
};

// Write octet at given byte-aligned bit offset
inline void pcm_aligned_write(uint8_t* buffer, size_t& offset, uint8_t arg) {
    buffer[offset >> 3] = arg;
    offset += 8;
}

// Read octet at given byte-aligned bit offset
inline uint8_t pcm_aligned_read(const uint8_t* buffer, size_t& offset) {
    uint8_t ret = buffer[offset >> 3];
    offset += 8;
    return ret;
}

// Write value (at most 8 bits) at given unaligned bit offset
inline void pcm_unaligned_write(uint8_t* buffer, size_t& offset, size_t length, uint8_t arg) {
    size_t byte_offset = (offset >> 3);
    size_t bit_offset = (offset & 0x7u);

    if (bit_offset == 0) {
        buffer[byte_offset] = 0;
    }

    buffer[byte_offset] |= uint8_t(arg << (8 - length) >> bit_offset);

    if (bit_offset + length > 8) {
        buffer[byte_offset + 1] = uint8_t(arg << bit_offset);
    }

    offset += length;
}

// Read value (at most 8 bits) at given unaligned bit offset
inline uint8_t pcm_unaligned_read(const uint8_t* buffer, size_t& offset, size_t length) {
    size_t byte_offset = (offset >> 3);
    size_t bit_offset = (offset & 0x7u);

    uint8_t ret = uint8_t(buffer[byte_offset] << bit_offset >> (8 - length));

    if (bit_offset + length > 8) {
        ret |= uint8_t(buffer[byte_offset + 1] >> (8 - bit_offset) >> (8 - length));
    }

    offset += length;
    return ret;
}

// Sample packer / unpacker
template <PcmEncoding, PcmEndian> struct pcm_packer;

// SInt8 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt8, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int8_t arg) {
        // native-endian view of octets
        pcm_sample<int8_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int8_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int8_t> p;

        // read in big-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// SInt8 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt8, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int8_t arg) {
        // native-endian view of octets
        pcm_sample<int8_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int8_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int8_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt8 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt8, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint8_t arg) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint8_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;

        // read in big-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt8 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt8, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint8_t arg) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint8_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// SInt16 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt16, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int16_t arg) {
        // native-endian view of octets
        pcm_sample<int16_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int16_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int16_t> p;

        // read in big-endian order
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// SInt16 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt16, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int16_t arg) {
        // native-endian view of octets
        pcm_sample<int16_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
    }

    // Unpack next sample from buffer
    static inline int16_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int16_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt16 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt16, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint16_t arg) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint16_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;

        // read in big-endian order
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt16 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt16, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint16_t arg) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
    }

    // Unpack next sample from buffer
    static inline uint16_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// SInt18 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt18, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, offset, 2, p.octets.octet2);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 2);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);


        if (p.value & 0x20000) {
            // sign extension
            p.value |= 0xfffc0000;
        }

        return p.value;
    }
};

// SInt18 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt18, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 2, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 2);
        p.octets.octet3 = 0;


        if (p.value & 0x20000) {
            // sign extension
            p.value |= 0xfffc0000;
        }

        return p.value;
    }
};

// UInt18 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt18, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, offset, 2, p.octets.octet2);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 2);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);

        return p.value;
    }
};

// UInt18 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt18, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 2, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 2);
        p.octets.octet3 = 0;

        return p.value;
    }
};

// SInt18_3B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt18_3B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= 0xfffc0000;
        }

        return p.value;
    }
};

// SInt18_3B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt18_3B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= 0xfffc0000;
        }

        return p.value;
    }
};

// UInt18_3B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt18_3B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0x3ffffu;
        return p.value;
    }
};

// UInt18_3B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt18_3B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0x3ffffu;
        return p.value;
    }
};

// SInt18_4B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt18_4B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= 0xfffc0000;
        }

        return p.value;
    }
};

// SInt18_4B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt18_4B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= 0xfffc0000;
        }

        return p.value;
    }
};

// UInt18_4B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt18_4B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0x3ffffu;
        return p.value;
    }
};

// UInt18_4B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt18_4B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0x3ffffu;
        return p.value;
    }
};

// SInt20 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt20, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, offset, 4, p.octets.octet2);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 4);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);


        if (p.value & 0x80000) {
            // sign extension
            p.value |= 0xfff00000;
        }

        return p.value;
    }
};

// SInt20 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt20, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 4, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 4);
        p.octets.octet3 = 0;


        if (p.value & 0x80000) {
            // sign extension
            p.value |= 0xfff00000;
        }

        return p.value;
    }
};

// UInt20 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt20, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, offset, 4, p.octets.octet2);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 4);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);

        return p.value;
    }
};

// UInt20 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt20, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, offset, 4, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, offset, 4);
        p.octets.octet3 = 0;

        return p.value;
    }
};

// SInt20_3B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt20_3B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= 0xfff00000;
        }

        return p.value;
    }
};

// SInt20_3B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt20_3B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= 0xfff00000;
        }

        return p.value;
    }
};

// UInt20_3B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt20_3B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xfffffu;
        return p.value;
    }
};

// UInt20_3B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt20_3B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0xfffffu;
        return p.value;
    }
};

// SInt20_4B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt20_4B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= 0xfff00000;
        }

        return p.value;
    }
};

// SInt20_4B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt20_4B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= 0xfff00000;
        }

        return p.value;
    }
};

// UInt20_4B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt20_4B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xfffffu;
        return p.value;
    }
};

// UInt20_4B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt20_4B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xfffffu;
        return p.value;
    }
};

// SInt24 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt24, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);


        if (p.value & 0x800000) {
            // sign extension
            p.value |= 0xff000000;
        }

        return p.value;
    }
};

// SInt24 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt24, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = 0;


        if (p.value & 0x800000) {
            // sign extension
            p.value |= 0xff000000;
        }

        return p.value;
    }
};

// UInt24 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt24, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt24 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt24, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = 0;

        return p.value;
    }
};

// SInt24_4B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt24_4B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffff;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xffffff;

        if (p.value & 0x800000) {
            // sign extension
            p.value |= 0xff000000;
        }

        return p.value;
    }
};

// SInt24_4B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt24_4B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffff;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xffffff;

        if (p.value & 0x800000) {
            // sign extension
            p.value |= 0xff000000;
        }

        return p.value;
    }
};

// UInt24_4B Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt24_4B, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xffffffu;
        return p.value;
    }
};

// UInt24_4B Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt24_4B, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        // zeroise padding bits
        p.value &= 0xffffffu;
        return p.value;
    }
};

// SInt32 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt32, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// SInt32 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt32, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt32 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt32, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt32 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt32, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// SInt64 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt64, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int64_t arg) {
        // native-endian view of octets
        pcm_sample<int64_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet7);
        pcm_aligned_write(buffer, offset, p.octets.octet6);
        pcm_aligned_write(buffer, offset, p.octets.octet5);
        pcm_aligned_write(buffer, offset, p.octets.octet4);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int64_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int64_t> p;

        // read in big-endian order
        p.octets.octet7 = pcm_aligned_read(buffer, offset);
        p.octets.octet6 = pcm_aligned_read(buffer, offset);
        p.octets.octet5 = pcm_aligned_read(buffer, offset);
        p.octets.octet4 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// SInt64 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_SInt64, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, int64_t arg) {
        // native-endian view of octets
        pcm_sample<int64_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet4);
        pcm_aligned_write(buffer, offset, p.octets.octet5);
        pcm_aligned_write(buffer, offset, p.octets.octet6);
        pcm_aligned_write(buffer, offset, p.octets.octet7);
    }

    // Unpack next sample from buffer
    static inline int64_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<int64_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet4 = pcm_aligned_read(buffer, offset);
        p.octets.octet5 = pcm_aligned_read(buffer, offset);
        p.octets.octet6 = pcm_aligned_read(buffer, offset);
        p.octets.octet7 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt64 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt64, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint64_t arg) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet7);
        pcm_aligned_write(buffer, offset, p.octets.octet6);
        pcm_aligned_write(buffer, offset, p.octets.octet5);
        pcm_aligned_write(buffer, offset, p.octets.octet4);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint64_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;

        // read in big-endian order
        p.octets.octet7 = pcm_aligned_read(buffer, offset);
        p.octets.octet6 = pcm_aligned_read(buffer, offset);
        p.octets.octet5 = pcm_aligned_read(buffer, offset);
        p.octets.octet4 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// UInt64 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_UInt64, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, uint64_t arg) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet4);
        pcm_aligned_write(buffer, offset, p.octets.octet5);
        pcm_aligned_write(buffer, offset, p.octets.octet6);
        pcm_aligned_write(buffer, offset, p.octets.octet7);
    }

    // Unpack next sample from buffer
    static inline uint64_t unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet4 = pcm_aligned_read(buffer, offset);
        p.octets.octet5 = pcm_aligned_read(buffer, offset);
        p.octets.octet6 = pcm_aligned_read(buffer, offset);
        p.octets.octet7 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// Float32 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_Float32, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, float arg) {
        // native-endian view of octets
        pcm_sample<float> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline float unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<float> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// Float32 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_Float32, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, float arg) {
        // native-endian view of octets
        pcm_sample<float> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline float unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<float> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// Float64 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_Float64, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, double arg) {
        // native-endian view of octets
        pcm_sample<double> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet7);
        pcm_aligned_write(buffer, offset, p.octets.octet6);
        pcm_aligned_write(buffer, offset, p.octets.octet5);
        pcm_aligned_write(buffer, offset, p.octets.octet4);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline double unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<double> p;

        // read in big-endian order
        p.octets.octet7 = pcm_aligned_read(buffer, offset);
        p.octets.octet6 = pcm_aligned_read(buffer, offset);
        p.octets.octet5 = pcm_aligned_read(buffer, offset);
        p.octets.octet4 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet0 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// Float64 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmEncoding_Float64, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& offset, double arg) {
        // native-endian view of octets
        pcm_sample<double> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, offset, p.octets.octet0);
        pcm_aligned_write(buffer, offset, p.octets.octet1);
        pcm_aligned_write(buffer, offset, p.octets.octet2);
        pcm_aligned_write(buffer, offset, p.octets.octet3);
        pcm_aligned_write(buffer, offset, p.octets.octet4);
        pcm_aligned_write(buffer, offset, p.octets.octet5);
        pcm_aligned_write(buffer, offset, p.octets.octet6);
        pcm_aligned_write(buffer, offset, p.octets.octet7);
    }

    // Unpack next sample from buffer
    static inline double unpack(const uint8_t* buffer, size_t& offset) {
        // native-endian view of octets
        pcm_sample<double> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, offset);
        p.octets.octet1 = pcm_aligned_read(buffer, offset);
        p.octets.octet2 = pcm_aligned_read(buffer, offset);
        p.octets.octet3 = pcm_aligned_read(buffer, offset);
        p.octets.octet4 = pcm_aligned_read(buffer, offset);
        p.octets.octet5 = pcm_aligned_read(buffer, offset);
        p.octets.octet6 = pcm_aligned_read(buffer, offset);
        p.octets.octet7 = pcm_aligned_read(buffer, offset);

        return p.value;
    }
};

// Map encoding and endian of samples
template <PcmEncoding InEnc, PcmEncoding OutEnc, PcmEndian InEnd, PcmEndian OutEnd>
struct pcm_mapper {
    static inline void map(const void* in_data, void* out_data, size_t n_samples) {
        const uint8_t* in = (const uint8_t*)in_data;
        uint8_t* out = (uint8_t*)out_data;

        size_t in_off = 0;
        size_t out_off = 0;

        for (size_t n = 0; n < n_samples; n++) {
            pcm_packer<OutEnc, OutEnd>::pack(out, out_off,
                pcm_encoding_converter<InEnc, OutEnc>::convert(
                    pcm_packer<InEnc, InEnd>::unpack(in, in_off)));
        }
    }
};

// Sample mapping function
typedef void (*pcm_mapper_func_t)(const void* in, void* out, size_t n_samples);

// Select mapper function
template <PcmEncoding InEnc, PcmEncoding OutEnc, PcmEndian InEnd, PcmEndian OutEnd>
pcm_mapper_func_t pcm_mapper_func() {
    return &pcm_mapper<InEnc, OutEnc, InEnd, OutEnd>::map;
}

// Select mapper function
template <PcmEncoding InEnc, PcmEncoding OutEnc, PcmEndian InEnd>
pcm_mapper_func_t pcm_mapper_func(PcmEndian out_endian) {
    switch (out_endian) {
    case PcmEndian_Native:
#if ROC_CPU_BIG_ENDIAN
        return pcm_mapper_func<InEnc, OutEnc, InEnd, PcmEndian_Big>();
#else
        return pcm_mapper_func<InEnc, OutEnc, InEnd, PcmEndian_Little>();
#endif
    case PcmEndian_Big:
        return pcm_mapper_func<InEnc, OutEnc, InEnd, PcmEndian_Big>();
    case PcmEndian_Little:
        return pcm_mapper_func<InEnc, OutEnc, InEnd, PcmEndian_Little>();
    }
    return NULL;
}

// Select mapper function
template <PcmEncoding InEnc, PcmEncoding OutEnc>
pcm_mapper_func_t pcm_mapper_func(PcmEndian in_endian, PcmEndian out_endian) {
    switch (in_endian) {
    case PcmEndian_Native:
#if ROC_CPU_BIG_ENDIAN
        return pcm_mapper_func<InEnc, OutEnc, PcmEndian_Big>(out_endian);
#else
        return pcm_mapper_func<InEnc, OutEnc, PcmEndian_Little>(out_endian);
#endif
    case PcmEndian_Big:
        return pcm_mapper_func<InEnc, OutEnc, PcmEndian_Big>(out_endian);
    case PcmEndian_Little:
        return pcm_mapper_func<InEnc, OutEnc, PcmEndian_Little>(out_endian);
    }
    return NULL;
}

// Select mapper function
template <PcmEncoding InEnc>
inline pcm_mapper_func_t pcm_mapper_func(PcmEncoding out_encoding,
                                         PcmEndian in_endian,
                                         PcmEndian out_endian) {
    switch (out_encoding) {
    case PcmEncoding_SInt8:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt8>(in_endian, out_endian);
    case PcmEncoding_UInt8:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt8>(in_endian, out_endian);
    case PcmEncoding_SInt16:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt16>(in_endian, out_endian);
    case PcmEncoding_UInt16:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt16>(in_endian, out_endian);
    case PcmEncoding_SInt18:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt18>(in_endian, out_endian);
    case PcmEncoding_UInt18:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt18>(in_endian, out_endian);
    case PcmEncoding_SInt18_3B:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt18_3B>(in_endian, out_endian);
    case PcmEncoding_UInt18_3B:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt18_3B>(in_endian, out_endian);
    case PcmEncoding_SInt18_4B:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt18_4B>(in_endian, out_endian);
    case PcmEncoding_UInt18_4B:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt18_4B>(in_endian, out_endian);
    case PcmEncoding_SInt20:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt20>(in_endian, out_endian);
    case PcmEncoding_UInt20:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt20>(in_endian, out_endian);
    case PcmEncoding_SInt20_3B:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt20_3B>(in_endian, out_endian);
    case PcmEncoding_UInt20_3B:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt20_3B>(in_endian, out_endian);
    case PcmEncoding_SInt20_4B:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt20_4B>(in_endian, out_endian);
    case PcmEncoding_UInt20_4B:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt20_4B>(in_endian, out_endian);
    case PcmEncoding_SInt24:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt24>(in_endian, out_endian);
    case PcmEncoding_UInt24:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt24>(in_endian, out_endian);
    case PcmEncoding_SInt24_4B:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt24_4B>(in_endian, out_endian);
    case PcmEncoding_UInt24_4B:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt24_4B>(in_endian, out_endian);
    case PcmEncoding_SInt32:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt32>(in_endian, out_endian);
    case PcmEncoding_UInt32:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt32>(in_endian, out_endian);
    case PcmEncoding_SInt64:
        return pcm_mapper_func<InEnc, PcmEncoding_SInt64>(in_endian, out_endian);
    case PcmEncoding_UInt64:
        return pcm_mapper_func<InEnc, PcmEncoding_UInt64>(in_endian, out_endian);
    case PcmEncoding_Float32:
        return pcm_mapper_func<InEnc, PcmEncoding_Float32>(in_endian, out_endian);
    case PcmEncoding_Float64:
        return pcm_mapper_func<InEnc, PcmEncoding_Float64>(in_endian, out_endian);
    }
    return NULL;
}

// Select mapper function
inline pcm_mapper_func_t pcm_mapper_func(PcmEncoding in_encoding,
                                         PcmEncoding out_encoding,
                                         PcmEndian in_endian,
                                         PcmEndian out_endian) {
    switch (in_encoding) {
    case PcmEncoding_SInt8:
        return pcm_mapper_func<PcmEncoding_SInt8>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt8:
        return pcm_mapper_func<PcmEncoding_UInt8>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt16:
        return pcm_mapper_func<PcmEncoding_SInt16>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt16:
        return pcm_mapper_func<PcmEncoding_UInt16>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt18:
        return pcm_mapper_func<PcmEncoding_SInt18>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt18:
        return pcm_mapper_func<PcmEncoding_UInt18>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt18_3B:
        return pcm_mapper_func<PcmEncoding_SInt18_3B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt18_3B:
        return pcm_mapper_func<PcmEncoding_UInt18_3B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt18_4B:
        return pcm_mapper_func<PcmEncoding_SInt18_4B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt18_4B:
        return pcm_mapper_func<PcmEncoding_UInt18_4B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt20:
        return pcm_mapper_func<PcmEncoding_SInt20>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt20:
        return pcm_mapper_func<PcmEncoding_UInt20>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt20_3B:
        return pcm_mapper_func<PcmEncoding_SInt20_3B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt20_3B:
        return pcm_mapper_func<PcmEncoding_UInt20_3B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt20_4B:
        return pcm_mapper_func<PcmEncoding_SInt20_4B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt20_4B:
        return pcm_mapper_func<PcmEncoding_UInt20_4B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt24:
        return pcm_mapper_func<PcmEncoding_SInt24>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt24:
        return pcm_mapper_func<PcmEncoding_UInt24>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt24_4B:
        return pcm_mapper_func<PcmEncoding_SInt24_4B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt24_4B:
        return pcm_mapper_func<PcmEncoding_UInt24_4B>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt32:
        return pcm_mapper_func<PcmEncoding_SInt32>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt32:
        return pcm_mapper_func<PcmEncoding_UInt32>(out_encoding, in_endian, out_endian);
    case PcmEncoding_SInt64:
        return pcm_mapper_func<PcmEncoding_SInt64>(out_encoding, in_endian, out_endian);
    case PcmEncoding_UInt64:
        return pcm_mapper_func<PcmEncoding_UInt64>(out_encoding, in_endian, out_endian);
    case PcmEncoding_Float32:
        return pcm_mapper_func<PcmEncoding_Float32>(out_encoding, in_endian, out_endian);
    case PcmEncoding_Float64:
        return pcm_mapper_func<PcmEncoding_Float64>(out_encoding, in_endian, out_endian);
    }
    return NULL;
}

// Get number of bits per sample in packed format
inline size_t pcm_sample_bits(PcmEncoding encoding) {
    switch (encoding) {
    case PcmEncoding_SInt8:
        return 8;
    case PcmEncoding_UInt8:
        return 8;
    case PcmEncoding_SInt16:
        return 16;
    case PcmEncoding_UInt16:
        return 16;
    case PcmEncoding_SInt18:
        return 18;
    case PcmEncoding_UInt18:
        return 18;
    case PcmEncoding_SInt18_3B:
        return 24;
    case PcmEncoding_UInt18_3B:
        return 24;
    case PcmEncoding_SInt18_4B:
        return 32;
    case PcmEncoding_UInt18_4B:
        return 32;
    case PcmEncoding_SInt20:
        return 20;
    case PcmEncoding_UInt20:
        return 20;
    case PcmEncoding_SInt20_3B:
        return 24;
    case PcmEncoding_UInt20_3B:
        return 24;
    case PcmEncoding_SInt20_4B:
        return 32;
    case PcmEncoding_UInt20_4B:
        return 32;
    case PcmEncoding_SInt24:
        return 24;
    case PcmEncoding_UInt24:
        return 24;
    case PcmEncoding_SInt24_4B:
        return 32;
    case PcmEncoding_UInt24_4B:
        return 32;
    case PcmEncoding_SInt32:
        return 32;
    case PcmEncoding_UInt32:
        return 32;
    case PcmEncoding_SInt64:
        return 64;
    case PcmEncoding_UInt64:
        return 64;
    case PcmEncoding_Float32:
        return 32;
    case PcmEncoding_Float64:
        return 64;
    }
    return 0;
}

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_MAPPER_FUNC_H_
