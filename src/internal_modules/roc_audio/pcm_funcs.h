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

// SInt18_3 value range
const int32_t pcm_sint18_3_min = -131071 - 1;
const int32_t pcm_sint18_3_max = 131071;

// UInt18_3 value range
const uint32_t pcm_uint18_3_min = 0u;
const uint32_t pcm_uint18_3_max = 262143u;

// SInt18_4 value range
const int32_t pcm_sint18_4_min = -131071 - 1;
const int32_t pcm_sint18_4_max = 131071;

// UInt18_4 value range
const uint32_t pcm_uint18_4_min = 0u;
const uint32_t pcm_uint18_4_max = 262143u;

// SInt20 value range
const int32_t pcm_sint20_min = -524287 - 1;
const int32_t pcm_sint20_max = 524287;

// UInt20 value range
const uint32_t pcm_uint20_min = 0u;
const uint32_t pcm_uint20_max = 1048575u;

// SInt20_3 value range
const int32_t pcm_sint20_3_min = -524287 - 1;
const int32_t pcm_sint20_3_max = 524287;

// UInt20_3 value range
const uint32_t pcm_uint20_3_min = 0u;
const uint32_t pcm_uint20_3_max = 1048575u;

// SInt20_4 value range
const int32_t pcm_sint20_4_min = -524287 - 1;
const int32_t pcm_sint20_4_max = 524287;

// UInt20_4 value range
const uint32_t pcm_uint20_4_min = 0u;
const uint32_t pcm_uint20_4_max = 1048575u;

// SInt24 value range
const int32_t pcm_sint24_min = -8388607 - 1;
const int32_t pcm_sint24_max = 8388607;

// UInt24 value range
const uint32_t pcm_uint24_min = 0u;
const uint32_t pcm_uint24_max = 16777215u;

// SInt24_4 value range
const int32_t pcm_sint24_4_min = -8388607 - 1;
const int32_t pcm_sint24_4_max = 8388607;

// UInt24_4 value range
const uint32_t pcm_uint24_4_min = 0u;
const uint32_t pcm_uint24_4_max = 16777215u;

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
template <PcmCode> struct pcm_sign_converter;

// Convert SInt8 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt8> {
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
template <> struct pcm_sign_converter<PcmCode_UInt8> {
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
        return int8_t(arg - uint8_t(pcm_sint8_max) - 1);
    }
};

// Convert SInt16 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt16> {
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
template <> struct pcm_sign_converter<PcmCode_UInt16> {
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
        return int16_t(arg - uint16_t(pcm_sint16_max) - 1);
    }
};

// Convert SInt18 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt18> {
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
template <> struct pcm_sign_converter<PcmCode_UInt18> {
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
        return int32_t(arg - uint32_t(pcm_sint18_max) - 1);
    }
};

// Convert SInt18_3 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt18_3> {
    // SInt18_3 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint18_3_max) + 1) {
            return int32_t(arg) - pcm_sint18_3_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint18_3_max) - 1);
    }

    // SInt18_3 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_3_max + 1;
        }
        return uint32_t(arg + pcm_sint18_3_max + 1);
    }
};

// Convert UInt18_3 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_UInt18_3> {
    // UInt18_3 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_3_max + 1;
        }
        return uint32_t(arg + pcm_sint18_3_max + 1);
    }

    // UInt18_3 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint18_3_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint18_3_max) - 1);
        }
        return int32_t(arg - uint32_t(pcm_sint18_3_max) - 1);
    }
};

// Convert SInt18_4 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt18_4> {
    // SInt18_4 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint18_4_max) + 1) {
            return int32_t(arg) - pcm_sint18_4_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint18_4_max) - 1);
    }

    // SInt18_4 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_4_max + 1;
        }
        return uint32_t(arg + pcm_sint18_4_max + 1);
    }
};

// Convert UInt18_4 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_UInt18_4> {
    // UInt18_4 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint18_4_max + 1;
        }
        return uint32_t(arg + pcm_sint18_4_max + 1);
    }

    // UInt18_4 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint18_4_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint18_4_max) - 1);
        }
        return int32_t(arg - uint32_t(pcm_sint18_4_max) - 1);
    }
};

// Convert SInt20 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt20> {
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
template <> struct pcm_sign_converter<PcmCode_UInt20> {
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
        return int32_t(arg - uint32_t(pcm_sint20_max) - 1);
    }
};

// Convert SInt20_3 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt20_3> {
    // SInt20_3 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint20_3_max) + 1) {
            return int32_t(arg) - pcm_sint20_3_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint20_3_max) - 1);
    }

    // SInt20_3 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_3_max + 1;
        }
        return uint32_t(arg + pcm_sint20_3_max + 1);
    }
};

// Convert UInt20_3 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_UInt20_3> {
    // UInt20_3 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_3_max + 1;
        }
        return uint32_t(arg + pcm_sint20_3_max + 1);
    }

    // UInt20_3 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint20_3_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint20_3_max) - 1);
        }
        return int32_t(arg - uint32_t(pcm_sint20_3_max) - 1);
    }
};

// Convert SInt20_4 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt20_4> {
    // SInt20_4 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint20_4_max) + 1) {
            return int32_t(arg) - pcm_sint20_4_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint20_4_max) - 1);
    }

    // SInt20_4 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_4_max + 1;
        }
        return uint32_t(arg + pcm_sint20_4_max + 1);
    }
};

// Convert UInt20_4 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_UInt20_4> {
    // UInt20_4 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint20_4_max + 1;
        }
        return uint32_t(arg + pcm_sint20_4_max + 1);
    }

    // UInt20_4 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint20_4_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint20_4_max) - 1);
        }
        return int32_t(arg - uint32_t(pcm_sint20_4_max) - 1);
    }
};

// Convert SInt24 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt24> {
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
template <> struct pcm_sign_converter<PcmCode_UInt24> {
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
        return int32_t(arg - uint32_t(pcm_sint24_max) - 1);
    }
};

// Convert SInt24_4 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt24_4> {
    // SInt24_4 from unsigned value
    static inline int32_t from_unsigned(uint32_t arg) {
        if (arg < uint32_t(pcm_sint24_4_max) + 1) {
            return int32_t(arg) - pcm_sint24_4_max - 1;
        }
        return int32_t(arg - uint32_t(pcm_sint24_4_max) - 1);
    }

    // SInt24_4 to unsigned value
    static inline uint32_t to_unsigned(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint24_4_max + 1;
        }
        return uint32_t(arg + pcm_sint24_4_max + 1);
    }
};

// Convert UInt24_4 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_UInt24_4> {
    // UInt24_4 from signed value
    static inline uint32_t from_signed(int32_t arg) {
        if (arg >= 0) {
            return uint32_t(arg) + pcm_sint24_4_max + 1;
        }
        return uint32_t(arg + pcm_sint24_4_max + 1);
    }

    // UInt24_4 to signed value
    static inline int32_t to_signed(uint32_t arg) {
        if (arg >= uint32_t(pcm_sint24_4_max) + 1) {
            return int32_t(arg - uint32_t(pcm_sint24_4_max) - 1);
        }
        return int32_t(arg - uint32_t(pcm_sint24_4_max) - 1);
    }
};

// Convert SInt32 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt32> {
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
template <> struct pcm_sign_converter<PcmCode_UInt32> {
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
        return int32_t(arg - uint32_t(pcm_sint32_max) - 1);
    }
};

// Convert SInt64 from/to signed/unsigned
template <> struct pcm_sign_converter<PcmCode_SInt64> {
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
template <> struct pcm_sign_converter<PcmCode_UInt64> {
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
        return int64_t(arg - uint64_t(pcm_sint64_max) - 1);
    }
};

// Convert between unpacked CODES
template <PcmCode InCode, PcmCode OutCode> struct pcm_code_converter;

// Convert SInt8 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt8> {
    static inline int8_t convert(int8_t arg) {
        return arg;
    }
};

// Convert UInt8 to SInt8
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt8> {
    static inline int8_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int8_t out;
        out = in;

        return out;
    }
};

// Convert SInt16 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt8> {
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
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt8> {
    static inline int8_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt8> {
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
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

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

// Convert SInt18_3 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert UInt18_3 to SInt8
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert SInt18_4 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert UInt18_4 to SInt8
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        return out;
    }
};

// Convert SInt20 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt8> {
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
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

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

// Convert SInt20_3 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt20_3 to SInt8
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt20_4 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt20_4 to SInt8
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt24 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt8> {
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
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

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

// Convert SInt24_4 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt8> {
    static inline int8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt8
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        return out;
    }
};

// Convert SInt32 to SInt8
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt8> {
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
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt8> {
    static inline int8_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt8> {
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
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt8> {
    static inline int8_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt8> {
    static inline int8_t convert(float arg) {
        float in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= (double)pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt8
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt8> {
    static inline int8_t convert(double arg) {
        double in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= (double)pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt8> {
    static inline uint8_t convert(int8_t arg) {
        int8_t in = arg;

        int8_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt8 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt8> {
    static inline uint8_t convert(uint8_t arg) {
        return arg;
    }
};

// Convert SInt16 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt8> {
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
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt16 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt8> {
    static inline uint8_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 8);

        return out;
    }
};

// Convert SInt18 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt8> {
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
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt18 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 10);

        return out;
    }
};

// Convert SInt18_3 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 10);

        return out;
    }
};

// Convert SInt18_4 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4_max - (int32_t(1) << 9))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 9)) >> 10);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 10);

        return out;
    }
};

// Convert SInt20 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt8> {
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
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt20 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 12);

        return out;
    }
};

// Convert SInt20_3 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 12);

        return out;
    }
};

// Convert SInt20_4 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 12);

        return out;
    }
};

// Convert SInt24 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt8> {
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
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt24 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 16);

        return out;
    }
};

// Convert SInt24_4 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt8> {
    static inline uint8_t convert(int32_t arg) {
        int32_t in = arg;

        int8_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 15))) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(uint32_t(in + (int32_t(1) << 15)) >> 16);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 16);

        return out;
    }
};

// Convert SInt32 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt8> {
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
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt32 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt8> {
    static inline uint8_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 24);

        return out;
    }
};

// Convert SInt64 to UInt8
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt8> {
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
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert UInt64 to UInt8
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt8> {
    static inline uint8_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint8_t out;
        // downscale unsigned integer
        out = uint8_t(in >> 56);

        return out;
    }
};

// Convert Float32 to UInt8
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt8> {
    static inline uint8_t convert(float arg) {
        float in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= (double)pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert Float64 to UInt8
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt8> {
    static inline uint8_t convert(double arg) {
        double in = arg;

        int8_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint8_max + 1.0);
        if (d < pcm_sint8_min) {
            // clip
            out = pcm_sint8_min;
        } else if (d >= (double)pcm_sint8_max + 1.0) {
            // clip
            out = pcm_sint8_max;
        } else {
            out = int8_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt8>::from_signed(out);
    }
};

// Convert SInt8 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt16> {
    static inline int16_t convert(int8_t arg) {
        int8_t in = arg;

        int16_t out;
        // upscale signed integer
        out = int16_t(uint16_t(in) << 8);

        return out;
    }
};

// Convert UInt8 to SInt16
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt16> {
    static inline int16_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int16_t out;
        // upscale signed integer
        out = int16_t(uint16_t(in) << 8);

        return out;
    }
};

// Convert SInt16 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt16> {
    static inline int16_t convert(int16_t arg) {
        return arg;
    }
};

// Convert UInt16 to SInt16
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt16> {
    static inline int16_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int16_t out;
        out = in;

        return out;
    }
};

// Convert SInt18 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt16> {
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
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

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

// Convert SInt18_3 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt18_3 to SInt16
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt18_4 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt18_4 to SInt16
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt16> {
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
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

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

// Convert SInt20_3 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt20_3 to SInt16
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt20_4 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt20_4 to SInt16
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt24 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt16> {
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
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

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

// Convert SInt24_4 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt16> {
    static inline int16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt16
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert SInt32 to SInt16
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt16> {
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
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt16> {
    static inline int16_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt16> {
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
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt16> {
    static inline int16_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt16> {
    static inline int16_t convert(float arg) {
        float in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= (double)pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt16
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt16> {
    static inline int16_t convert(double arg) {
        double in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= (double)pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt16> {
    static inline uint16_t convert(int8_t arg) {
        int8_t in = arg;

        int16_t out;
        // upscale signed integer
        out = int16_t(uint16_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt8 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt16> {
    static inline uint16_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint16_t out;
        // upscale unsigned integer
        out = uint16_t(uint16_t(in) << 8);

        return out;
    }
};

// Convert SInt16 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt16> {
    static inline uint16_t convert(int16_t arg) {
        int16_t in = arg;

        int16_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt16 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt16> {
    static inline uint16_t convert(uint16_t arg) {
        return arg;
    }
};

// Convert SInt18 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt16> {
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
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt18 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 2);

        return out;
    }
};

// Convert SInt18_3 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 2);

        return out;
    }
};

// Convert SInt18_4 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint18_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 2);

        return out;
    }
};

// Convert SInt20 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt16> {
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
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt20 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 4);

        return out;
    }
};

// Convert SInt20_3 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 4);

        return out;
    }
};

// Convert SInt20_4 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 4);

        return out;
    }
};

// Convert SInt24 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt16> {
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
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt24 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 8);

        return out;
    }
};

// Convert SInt24_4 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt16> {
    static inline uint16_t convert(int32_t arg) {
        int32_t in = arg;

        int16_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 8);

        return out;
    }
};

// Convert SInt32 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt16> {
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
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt32 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt16> {
    static inline uint16_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 16);

        return out;
    }
};

// Convert SInt64 to UInt16
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt16> {
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
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert UInt64 to UInt16
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt16> {
    static inline uint16_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint16_t out;
        // downscale unsigned integer
        out = uint16_t(in >> 48);

        return out;
    }
};

// Convert Float32 to UInt16
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt16> {
    static inline uint16_t convert(float arg) {
        float in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= (double)pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert Float64 to UInt16
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt16> {
    static inline uint16_t convert(double arg) {
        double in = arg;

        int16_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint16_max + 1.0);
        if (d < pcm_sint16_min) {
            // clip
            out = pcm_sint16_min;
        } else if (d >= (double)pcm_sint16_max + 1.0) {
            // clip
            out = pcm_sint16_max;
        } else {
            out = int16_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt16>::from_signed(out);
    }
};

// Convert SInt8 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt18> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert UInt8 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt18> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt18> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt16 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt18> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt18> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt18 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_3 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_4 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt18> {
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
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

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

// Convert SInt20_3 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_3 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_4 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_4 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt24 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt18> {
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
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

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

// Convert SInt24_4 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt18> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt18
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt32 to SInt18
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt18> {
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
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt18> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt18> {
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
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt18> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt18> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= (double)pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt18
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt18> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= (double)pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt18> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt8 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt18> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt18> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt16 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt18> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt18 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt18_3 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt18> {
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
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt20 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_3 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_4 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt24 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt18> {
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
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt24 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt24_4 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt18> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt32 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt18> {
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
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt32 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt18> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 14);

        return out;
    }
};

// Convert SInt64 to UInt18
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt18> {
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
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert UInt64 to UInt18
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt18> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 46);

        return out;
    }
};

// Convert Float32 to UInt18
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt18> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= (double)pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert Float64 to UInt18
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt18> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_max + 1.0);
        if (d < pcm_sint18_min) {
            // clip
            out = pcm_sint18_min;
        } else if (d >= (double)pcm_sint18_max + 1.0) {
            // clip
            out = pcm_sint18_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18>::from_signed(out);
    }
};

// Convert SInt8 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt18_3> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert UInt8 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt18_3> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt18_3> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt16 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt18_3> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt18_3 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_4 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_3 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_3 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_4 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_4 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt24 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt24_4 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt32 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt18_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert UInt32 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt18_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert SInt64 to SInt18_3
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt18_3> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert UInt64 to SInt18_3
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt18_3> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert Float32 to SInt18_3
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt18_3> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_3_max + 1.0);
        if (d < pcm_sint18_3_min) {
            // clip
            out = pcm_sint18_3_min;
        } else if (d >= (double)pcm_sint18_3_max + 1.0) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt18_3
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt18_3> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_3_max + 1.0);
        if (d < pcm_sint18_3_min) {
            // clip
            out = pcm_sint18_3_min;
        } else if (d >= (double)pcm_sint18_3_max + 1.0) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt18_3> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt8 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt18_3> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt16 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt18 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt18_4 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt20 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_3 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_4 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt24 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt24 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt24_4 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt32 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt18_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt32 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 14);

        return out;
    }
};

// Convert SInt64 to UInt18_3
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt18_3> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert UInt64 to UInt18_3
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt18_3> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 46);

        return out;
    }
};

// Convert Float32 to UInt18_3
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt18_3> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_3_max + 1.0);
        if (d < pcm_sint18_3_min) {
            // clip
            out = pcm_sint18_3_min;
        } else if (d >= (double)pcm_sint18_3_max + 1.0) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert Float64 to UInt18_3
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt18_3> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_3_max + 1.0);
        if (d < pcm_sint18_3_min) {
            // clip
            out = pcm_sint18_3_min;
        } else if (d >= (double)pcm_sint18_3_max + 1.0) {
            // clip
            out = pcm_sint18_3_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_3>::from_signed(out);
    }
};

// Convert SInt8 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt18_4> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert UInt8 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt18_4> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt18_4> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt16 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt18_4> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt18_3 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt18_4 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_3 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_3 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt20_4 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert UInt20_4 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        return out;
    }
};

// Convert SInt24 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt24_4 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        return out;
    }
};

// Convert SInt32 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt18_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert UInt32 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt18_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        return out;
    }
};

// Convert SInt64 to SInt18_4
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt18_4> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert UInt64 to SInt18_4
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt18_4> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        return out;
    }
};

// Convert Float32 to SInt18_4
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt18_4> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_4_max + 1.0);
        if (d < pcm_sint18_4_min) {
            // clip
            out = pcm_sint18_4_min;
        } else if (d >= (double)pcm_sint18_4_max + 1.0) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt18_4
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt18_4> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_4_max + 1.0);
        if (d < pcm_sint18_4_min) {
            // clip
            out = pcm_sint18_4_min;
        } else if (d >= (double)pcm_sint18_4_max + 1.0) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt18_4> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 10);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt8 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 10);

        return out;
    }
};

// Convert SInt16 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt18_4> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt16 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt18 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_3 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt18_4 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt20 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt20 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_3 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_3_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt20_4 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint20_4_max - (int32_t(1) << 1))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 1)) >> 2);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 2);

        return out;
    }
};

// Convert SInt24 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt24 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt24_4 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 5))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 5)) >> 6);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 6);

        return out;
    }
};

// Convert SInt32 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt18_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 13))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 13)) >> 14);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt32 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 14);

        return out;
    }
};

// Convert SInt64 to UInt18_4
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt18_4> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 45))) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 45)) >> 46);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert UInt64 to UInt18_4
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt18_4> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 46);

        return out;
    }
};

// Convert Float32 to UInt18_4
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt18_4> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_4_max + 1.0);
        if (d < pcm_sint18_4_min) {
            // clip
            out = pcm_sint18_4_min;
        } else if (d >= (double)pcm_sint18_4_max + 1.0) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert Float64 to UInt18_4
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt18_4> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint18_4_max + 1.0);
        if (d < pcm_sint18_4_min) {
            // clip
            out = pcm_sint18_4_min;
        } else if (d >= (double)pcm_sint18_4_max + 1.0) {
            // clip
            out = pcm_sint18_4_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt18_4>::from_signed(out);
    }
};

// Convert SInt8 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt20> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt8 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt20> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt20> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt16 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt20> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_3 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_4 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt20> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt20 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_3 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_4 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt20> {
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
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

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

// Convert SInt24_4 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt20> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt20
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt32 to SInt20
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt20> {
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
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt20> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt20> {
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
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt20> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt20> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= (double)pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt20
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt20> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= (double)pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt20> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt8 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt20> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt20> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt16 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt20> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt18 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt20 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt20_3 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt20> {
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
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt24 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt24_4 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt20> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt32 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt20> {
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
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt32 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt20> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 12);

        return out;
    }
};

// Convert SInt64 to UInt20
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt20> {
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
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert UInt64 to UInt20
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt20> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 44);

        return out;
    }
};

// Convert Float32 to UInt20
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt20> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= (double)pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert Float64 to UInt20
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt20> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_max + 1.0);
        if (d < pcm_sint20_min) {
            // clip
            out = pcm_sint20_min;
        } else if (d >= (double)pcm_sint20_max + 1.0) {
            // clip
            out = pcm_sint20_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20>::from_signed(out);
    }
};

// Convert SInt8 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt20_3> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt8 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt20_3> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt20_3> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt16 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt20_3> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_3 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_4 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt20_3 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_4 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt24_4 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt32 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt20_3> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt32 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt20_3> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt64 to SInt20_3
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt20_3> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert UInt64 to SInt20_3
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt20_3> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert Float32 to SInt20_3
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt20_3> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_3_max + 1.0);
        if (d < pcm_sint20_3_min) {
            // clip
            out = pcm_sint20_3_min;
        } else if (d >= (double)pcm_sint20_3_max + 1.0) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt20_3
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt20_3> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_3_max + 1.0);
        if (d < pcm_sint20_3_min) {
            // clip
            out = pcm_sint20_3_min;
        } else if (d >= (double)pcm_sint20_3_max + 1.0) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt20_3> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt8 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt20_3> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt16 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt18 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt20 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt20_4 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt24 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt24_4 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt32 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt20_3> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt32 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 12);

        return out;
    }
};

// Convert SInt64 to UInt20_3
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt20_3> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert UInt64 to UInt20_3
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt20_3> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 44);

        return out;
    }
};

// Convert Float32 to UInt20_3
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt20_3> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_3_max + 1.0);
        if (d < pcm_sint20_3_min) {
            // clip
            out = pcm_sint20_3_min;
        } else if (d >= (double)pcm_sint20_3_max + 1.0) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert Float64 to UInt20_3
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt20_3> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_3_max + 1.0);
        if (d < pcm_sint20_3_min) {
            // clip
            out = pcm_sint20_3_min;
        } else if (d >= (double)pcm_sint20_3_max + 1.0) {
            // clip
            out = pcm_sint20_3_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_3>::from_signed(out);
    }
};

// Convert SInt8 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt20_4> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt8 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt20_4> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt20_4> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt16 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt20_4> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_3 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert UInt18_4 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt20_3 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt20_4 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt24_4 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert UInt24_4 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        return out;
    }
};

// Convert SInt32 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt20_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert UInt32 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt20_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        return out;
    }
};

// Convert SInt64 to SInt20_4
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt20_4> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert UInt64 to SInt20_4
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt20_4> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        return out;
    }
};

// Convert Float32 to SInt20_4
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt20_4> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_4_max + 1.0);
        if (d < pcm_sint20_4_min) {
            // clip
            out = pcm_sint20_4_min;
        } else if (d >= (double)pcm_sint20_4_max + 1.0) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt20_4
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt20_4> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_4_max + 1.0);
        if (d < pcm_sint20_4_min) {
            // clip
            out = pcm_sint20_4_min;
        } else if (d >= (double)pcm_sint20_4_max + 1.0) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt20_4> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt8 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt16 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt20_4> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt16 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt18 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt18 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_3 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt18_4 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 2);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 2);

        return out;
    }
};

// Convert SInt20 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt20 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_3 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt20_4 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt24 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt24 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt24_4 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint24_4_max - (int32_t(1) << 3))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 3)) >> 4);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 4);

        return out;
    }
};

// Convert SInt32 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt20_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 11))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 11)) >> 12);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt32 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 12);

        return out;
    }
};

// Convert SInt64 to UInt20_4
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt20_4> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 43))) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 43)) >> 44);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert UInt64 to UInt20_4
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt20_4> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 44);

        return out;
    }
};

// Convert Float32 to UInt20_4
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt20_4> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_4_max + 1.0);
        if (d < pcm_sint20_4_min) {
            // clip
            out = pcm_sint20_4_min;
        } else if (d >= (double)pcm_sint20_4_max + 1.0) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert Float64 to UInt20_4
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt20_4> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint20_4_max + 1.0);
        if (d < pcm_sint20_4_min) {
            // clip
            out = pcm_sint20_4_min;
        } else if (d >= (double)pcm_sint20_4_max + 1.0) {
            // clip
            out = pcm_sint20_4_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt20_4>::from_signed(out);
    }
};

// Convert SInt8 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt24> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert UInt8 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt24> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt24> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt16 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt24> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_3 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_4 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_3 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_4 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt24 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24_4 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt24> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt24_4 to SInt24
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt32 to SInt24
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt24> {
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
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt24> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt24> {
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
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt24> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt24> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= (double)pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt24
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt24> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= (double)pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt24> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt8 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt24> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt24> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt16 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt24> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt18 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt20 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt24 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt24_4 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt24> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt32 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt24> {
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
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt32 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt24> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 8);

        return out;
    }
};

// Convert SInt64 to UInt24
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt24> {
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
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert UInt64 to UInt24
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt24> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 40);

        return out;
    }
};

// Convert Float32 to UInt24
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt24> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= (double)pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert Float64 to UInt24
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt24> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_max + 1.0);
        if (d < pcm_sint24_min) {
            // clip
            out = pcm_sint24_min;
        } else if (d >= (double)pcm_sint24_max + 1.0) {
            // clip
            out = pcm_sint24_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24>::from_signed(out);
    }
};

// Convert SInt8 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt24_4> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert UInt8 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt24_4> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt24_4> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt16 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt24_4> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_3 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert UInt18_4 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_3 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert UInt20_4 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        return out;
    }
};

// Convert UInt24 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24_4 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt24_4 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt32 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt24_4> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert UInt32 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt24_4> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        return out;
    }
};

// Convert SInt64 to SInt24_4
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt24_4> {
    static inline int32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        return out;
    }
};

// Convert UInt64 to SInt24_4
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt24_4> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        return out;
    }
};

// Convert Float32 to SInt24_4
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt24_4> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_4_max + 1.0);
        if (d < pcm_sint24_4_min) {
            // clip
            out = pcm_sint24_4_min;
        } else if (d >= (double)pcm_sint24_4_max + 1.0) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt24_4
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt24_4> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_4_max + 1.0);
        if (d < pcm_sint24_4_min) {
            // clip
            out = pcm_sint24_4_min;
        } else if (d >= (double)pcm_sint24_4_max + 1.0) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt24_4> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt8 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt16 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt24_4> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt16 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt18 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt18 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_3 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt18_4 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 6);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 6);

        return out;
    }
};

// Convert SInt20 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt20 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_3 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt20_4 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 4);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 4);

        return out;
    }
};

// Convert SInt24 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt24 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        out = in;

        return out;
    }
};

// Convert SInt24_4 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt32 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt24_4> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int32_t(pcm_sint32_max - (int32_t(1) << 7))) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(uint32_t(in + (int32_t(1) << 7)) >> 8);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt32 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 8);

        return out;
    }
};

// Convert SInt64 to UInt24_4
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt24_4> {
    static inline uint32_t convert(int64_t arg) {
        int64_t in = arg;

        int32_t out;
        // downscale signed integer
        if (in > int64_t(pcm_sint64_max - (int64_t(1) << 39))) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(uint64_t(in + (int64_t(1) << 39)) >> 40);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert UInt64 to UInt24_4
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt24_4> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 40);

        return out;
    }
};

// Convert Float32 to UInt24_4
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt24_4> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_4_max + 1.0);
        if (d < pcm_sint24_4_min) {
            // clip
            out = pcm_sint24_4_min;
        } else if (d >= (double)pcm_sint24_4_max + 1.0) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert Float64 to UInt24_4
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt24_4> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint24_4_max + 1.0);
        if (d < pcm_sint24_4_min) {
            // clip
            out = pcm_sint24_4_min;
        } else if (d >= (double)pcm_sint24_4_max + 1.0) {
            // clip
            out = pcm_sint24_4_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt24_4>::from_signed(out);
    }
};

// Convert SInt8 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt32> {
    static inline int32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 24);

        return out;
    }
};

// Convert UInt8 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt32> {
    static inline int32_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 24);

        return out;
    }
};

// Convert SInt16 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt32> {
    static inline int32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert UInt16 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt32> {
    static inline int32_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt18 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert UInt18 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_3 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert UInt18_3 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_4 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert UInt18_4 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt20 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt20 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_3 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt20_3 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_4 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert UInt20_4 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt24 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt24 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt24_4 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert UInt24_4 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt32 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt32> {
    static inline int32_t convert(int32_t arg) {
        return arg;
    }
};

// Convert UInt32 to SInt32
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt32> {
    static inline int32_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        int32_t out;
        out = in;

        return out;
    }
};

// Convert SInt64 to SInt32
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt32> {
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
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt32> {
    static inline int32_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

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
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt32> {
    static inline int32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= (double)pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt32
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt32> {
    static inline int32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= (double)pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt32> {
    static inline uint32_t convert(int8_t arg) {
        int8_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 24);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt8 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt32> {
    static inline uint32_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 24);

        return out;
    }
};

// Convert SInt16 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt32> {
    static inline uint32_t convert(int16_t arg) {
        int16_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 16);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt16 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt32> {
    static inline uint32_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 16);

        return out;
    }
};

// Convert SInt18 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt18 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_3 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt18_4 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 14);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 14);

        return out;
    }
};

// Convert SInt20 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt20 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_3 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt20_4 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 12);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 12);

        return out;
    }
};

// Convert SInt24 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt24 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt24_4 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        // upscale signed integer
        out = int32_t(uint32_t(in) << 8);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint32_t out;
        // upscale unsigned integer
        out = uint32_t(uint32_t(in) << 8);

        return out;
    }
};

// Convert SInt32 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt32> {
    static inline uint32_t convert(int32_t arg) {
        int32_t in = arg;

        int32_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt32 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt32> {
    static inline uint32_t convert(uint32_t arg) {
        return arg;
    }
};

// Convert SInt64 to UInt32
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt32> {
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
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert UInt64 to UInt32
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt32> {
    static inline uint32_t convert(uint64_t arg) {
        uint64_t in = arg;

        uint32_t out;
        // downscale unsigned integer
        out = uint32_t(in >> 32);

        return out;
    }
};

// Convert Float32 to UInt32
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt32> {
    static inline uint32_t convert(float arg) {
        float in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= (double)pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert Float64 to UInt32
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt32> {
    static inline uint32_t convert(double arg) {
        double in = arg;

        int32_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint32_max + 1.0);
        if (d < pcm_sint32_min) {
            // clip
            out = pcm_sint32_min;
        } else if (d >= (double)pcm_sint32_max + 1.0) {
            // clip
            out = pcm_sint32_max;
        } else {
            out = int32_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt32>::from_signed(out);
    }
};

// Convert SInt8 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_SInt64> {
    static inline int64_t convert(int8_t arg) {
        int8_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 56);

        return out;
    }
};

// Convert UInt8 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_SInt64> {
    static inline int64_t convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 56);

        return out;
    }
};

// Convert SInt16 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_SInt64> {
    static inline int64_t convert(int16_t arg) {
        int16_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 48);

        return out;
    }
};

// Convert UInt16 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_SInt64> {
    static inline int64_t convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 48);

        return out;
    }
};

// Convert SInt18 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert UInt18 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_3 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert UInt18_3 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_4 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert UInt18_4 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt20 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert UInt20 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_3 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert UInt20_3 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_4 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert UInt20_4 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt24 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert UInt24 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt24_4 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert UInt24_4 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt32 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_SInt64> {
    static inline int64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 32);

        return out;
    }
};

// Convert UInt32 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_SInt64> {
    static inline int64_t convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 32);

        return out;
    }
};

// Convert SInt64 to SInt64
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_SInt64> {
    static inline int64_t convert(int64_t arg) {
        return arg;
    }
};

// Convert UInt64 to SInt64
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_SInt64> {
    static inline int64_t convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        int64_t out;
        out = in;

        return out;
    }
};

// Convert Float32 to SInt64
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_SInt64> {
    static inline int64_t convert(float arg) {
        float in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= (double)pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        return out;
    }
};

// Convert Float64 to SInt64
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_SInt64> {
    static inline int64_t convert(double arg) {
        double in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= (double)pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        return out;
    }
};

// Convert SInt8 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_UInt64> {
    static inline uint64_t convert(int8_t arg) {
        int8_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 56);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt8 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_UInt64> {
    static inline uint64_t convert(uint8_t arg) {
        uint8_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 56);

        return out;
    }
};

// Convert SInt16 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_UInt64> {
    static inline uint64_t convert(int16_t arg) {
        int16_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 48);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt16 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_UInt64> {
    static inline uint64_t convert(uint16_t arg) {
        uint16_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 48);

        return out;
    }
};

// Convert SInt18 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt18 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_3 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt18_3 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt18_4 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 46);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt18_4 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 46);

        return out;
    }
};

// Convert SInt20 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt20 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_3 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt20_3 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt20_4 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 44);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt20_4 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 44);

        return out;
    }
};

// Convert SInt24 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt24 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt24_4 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 40);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt24_4 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 40);

        return out;
    }
};

// Convert SInt32 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_UInt64> {
    static inline uint64_t convert(int32_t arg) {
        int32_t in = arg;

        int64_t out;
        // upscale signed integer
        out = int64_t(uint64_t(in) << 32);

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt32 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_UInt64> {
    static inline uint64_t convert(uint32_t arg) {
        uint32_t in = arg;

        uint64_t out;
        // upscale unsigned integer
        out = uint64_t(uint64_t(in) << 32);

        return out;
    }
};

// Convert SInt64 to UInt64
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_UInt64> {
    static inline uint64_t convert(int64_t arg) {
        int64_t in = arg;

        int64_t out;
        out = in;

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert UInt64 to UInt64
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_UInt64> {
    static inline uint64_t convert(uint64_t arg) {
        return arg;
    }
};

// Convert Float32 to UInt64
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_UInt64> {
    static inline uint64_t convert(float arg) {
        float in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= (double)pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert Float64 to UInt64
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_UInt64> {
    static inline uint64_t convert(double arg) {
        double in = arg;

        int64_t out;
        // float to integer
        const double d = double(in) * ((double)pcm_sint64_max + 1.0);
        if (d < pcm_sint64_min) {
            // clip
            out = pcm_sint64_min;
        } else if (d >= (double)pcm_sint64_max + 1.0) {
            // clip
            out = pcm_sint64_max;
        } else {
            out = int64_t(d);
        }

        // convert to unsigned
        return pcm_sign_converter<PcmCode_UInt64>::from_signed(out);
    }
};

// Convert SInt8 to Float32
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_Float32> {
    static inline float convert(int8_t arg) {
        int8_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert UInt8 to Float32
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_Float32> {
    static inline float convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert SInt16 to Float32
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_Float32> {
    static inline float convert(int16_t arg) {
        int16_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert UInt16 to Float32
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_Float32> {
    static inline float convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert SInt18 to Float32
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert UInt18 to Float32
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert SInt18_3 to Float32
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint18_3_max + 1.0)));

        return out;
    }
};

// Convert UInt18_3 to Float32
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint18_3_max + 1.0)));

        return out;
    }
};

// Convert SInt18_4 to Float32
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint18_4_max + 1.0)));

        return out;
    }
};

// Convert UInt18_4 to Float32
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint18_4_max + 1.0)));

        return out;
    }
};

// Convert SInt20 to Float32
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert UInt20 to Float32
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert SInt20_3 to Float32
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint20_3_max + 1.0)));

        return out;
    }
};

// Convert UInt20_3 to Float32
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint20_3_max + 1.0)));

        return out;
    }
};

// Convert SInt20_4 to Float32
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint20_4_max + 1.0)));

        return out;
    }
};

// Convert UInt20_4 to Float32
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint20_4_max + 1.0)));

        return out;
    }
};

// Convert SInt24 to Float32
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert UInt24 to Float32
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert SInt24_4 to Float32
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint24_4_max + 1.0)));

        return out;
    }
};

// Convert UInt24_4 to Float32
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint24_4_max + 1.0)));

        return out;
    }
};

// Convert SInt32 to Float32
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_Float32> {
    static inline float convert(int32_t arg) {
        int32_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert UInt32 to Float32
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_Float32> {
    static inline float convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert SInt64 to Float32
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_Float32> {
    static inline float convert(int64_t arg) {
        int64_t in = arg;

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert UInt64 to Float32
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_Float32> {
    static inline float convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        float out;
        // integer to float
        out = float(in * (1.0 / ((double)pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert Float32 to Float32
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_Float32> {
    static inline float convert(float arg) {
        return arg;
    }
};

// Convert Float64 to Float32
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_Float32> {
    static inline float convert(double arg) {
        double in = arg;

        float out;
        // float to float
        out = float(in);

        return out;
    }
};

// Convert SInt8 to Float64
template <> struct pcm_code_converter<PcmCode_SInt8, PcmCode_Float64> {
    static inline double convert(int8_t arg) {
        int8_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert UInt8 to Float64
template <> struct pcm_code_converter<PcmCode_UInt8, PcmCode_Float64> {
    static inline double convert(uint8_t arg) {
        // convert to signed
        int8_t in = pcm_sign_converter<PcmCode_UInt8>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint8_max + 1.0)));

        return out;
    }
};

// Convert SInt16 to Float64
template <> struct pcm_code_converter<PcmCode_SInt16, PcmCode_Float64> {
    static inline double convert(int16_t arg) {
        int16_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert UInt16 to Float64
template <> struct pcm_code_converter<PcmCode_UInt16, PcmCode_Float64> {
    static inline double convert(uint16_t arg) {
        // convert to signed
        int16_t in = pcm_sign_converter<PcmCode_UInt16>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint16_max + 1.0)));

        return out;
    }
};

// Convert SInt18 to Float64
template <> struct pcm_code_converter<PcmCode_SInt18, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert UInt18 to Float64
template <> struct pcm_code_converter<PcmCode_UInt18, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint18_max + 1.0)));

        return out;
    }
};

// Convert SInt18_3 to Float64
template <> struct pcm_code_converter<PcmCode_SInt18_3, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint18_3_max + 1.0)));

        return out;
    }
};

// Convert UInt18_3 to Float64
template <> struct pcm_code_converter<PcmCode_UInt18_3, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_3>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint18_3_max + 1.0)));

        return out;
    }
};

// Convert SInt18_4 to Float64
template <> struct pcm_code_converter<PcmCode_SInt18_4, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint18_4_max + 1.0)));

        return out;
    }
};

// Convert UInt18_4 to Float64
template <> struct pcm_code_converter<PcmCode_UInt18_4, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt18_4>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint18_4_max + 1.0)));

        return out;
    }
};

// Convert SInt20 to Float64
template <> struct pcm_code_converter<PcmCode_SInt20, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert UInt20 to Float64
template <> struct pcm_code_converter<PcmCode_UInt20, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint20_max + 1.0)));

        return out;
    }
};

// Convert SInt20_3 to Float64
template <> struct pcm_code_converter<PcmCode_SInt20_3, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint20_3_max + 1.0)));

        return out;
    }
};

// Convert UInt20_3 to Float64
template <> struct pcm_code_converter<PcmCode_UInt20_3, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_3>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint20_3_max + 1.0)));

        return out;
    }
};

// Convert SInt20_4 to Float64
template <> struct pcm_code_converter<PcmCode_SInt20_4, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint20_4_max + 1.0)));

        return out;
    }
};

// Convert UInt20_4 to Float64
template <> struct pcm_code_converter<PcmCode_UInt20_4, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt20_4>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint20_4_max + 1.0)));

        return out;
    }
};

// Convert SInt24 to Float64
template <> struct pcm_code_converter<PcmCode_SInt24, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert UInt24 to Float64
template <> struct pcm_code_converter<PcmCode_UInt24, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint24_max + 1.0)));

        return out;
    }
};

// Convert SInt24_4 to Float64
template <> struct pcm_code_converter<PcmCode_SInt24_4, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint24_4_max + 1.0)));

        return out;
    }
};

// Convert UInt24_4 to Float64
template <> struct pcm_code_converter<PcmCode_UInt24_4, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt24_4>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint24_4_max + 1.0)));

        return out;
    }
};

// Convert SInt32 to Float64
template <> struct pcm_code_converter<PcmCode_SInt32, PcmCode_Float64> {
    static inline double convert(int32_t arg) {
        int32_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert UInt32 to Float64
template <> struct pcm_code_converter<PcmCode_UInt32, PcmCode_Float64> {
    static inline double convert(uint32_t arg) {
        // convert to signed
        int32_t in = pcm_sign_converter<PcmCode_UInt32>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint32_max + 1.0)));

        return out;
    }
};

// Convert SInt64 to Float64
template <> struct pcm_code_converter<PcmCode_SInt64, PcmCode_Float64> {
    static inline double convert(int64_t arg) {
        int64_t in = arg;

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert UInt64 to Float64
template <> struct pcm_code_converter<PcmCode_UInt64, PcmCode_Float64> {
    static inline double convert(uint64_t arg) {
        // convert to signed
        int64_t in = pcm_sign_converter<PcmCode_UInt64>::to_signed(arg);

        double out;
        // integer to float
        out = double(in * (1.0 / ((double)pcm_sint64_max + 1.0)));

        return out;
    }
};

// Convert Float32 to Float64
template <> struct pcm_code_converter<PcmCode_Float32, PcmCode_Float64> {
    static inline double convert(float arg) {
        float in = arg;

        double out;
        // float to float
        out = double(in);

        return out;
    }
};

// Convert Float64 to Float64
template <> struct pcm_code_converter<PcmCode_Float64, PcmCode_Float64> {
    static inline double convert(double arg) {
        return arg;
    }
};

// N-byte native-endian packed octet array
template <size_t N> struct pcm_octets;

// 1-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<1> {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    uint8_t octet0;
#else
    uint8_t octet0;
#endif
} ROC_ATTR_PACKED_END;

// 2-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<2> {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    uint8_t octet1;
    uint8_t octet0;
#else
    uint8_t octet0;
    uint8_t octet1;
#endif
} ROC_ATTR_PACKED_END;

// 4-byte native-endian packed octet array
template <> ROC_ATTR_PACKED_BEGIN struct pcm_octets<4> {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
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
#if ROC_CPU_ENDIAN == ROC_CPU_BE
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

// SInt8 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt8, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int8_t arg) {
        // native-endian view of octets
        pcm_sample<int8_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int8_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int8_t> p;

        // read in big-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// SInt8 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt8, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int8_t arg) {
        // native-endian view of octets
        pcm_sample<int8_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int8_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int8_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt8 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt8, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint8_t arg) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint8_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;

        // read in big-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt8 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt8, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint8_t arg) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint8_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint8_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// SInt16 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt16, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int16_t arg) {
        // native-endian view of octets
        pcm_sample<int16_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int16_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int16_t> p;

        // read in big-endian order
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// SInt16 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt16, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int16_t arg) {
        // native-endian view of octets
        pcm_sample<int16_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
    }

    // Unpack next sample from buffer
    static inline int16_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int16_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt16 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt16, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint16_t arg) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint16_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;

        // read in big-endian order
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt16 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt16, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint16_t arg) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
    }

    // Unpack next sample from buffer
    static inline uint16_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint16_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// SInt18 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt18, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, bit_offset, 2, p.octets.octet2);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 2);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);

        if (p.value & 0x20000) {
            // sign extension
            p.value |= (int32_t)0xfffc0000;
        }

        return p.value;
    }
};

// SInt18 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt18, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 2, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 2);
        p.octets.octet3 = 0;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= (int32_t)0xfffc0000;
        }

        return p.value;
    }
};

// UInt18 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt18, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, bit_offset, 2, p.octets.octet2);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 2);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);

        return p.value;
    }
};

// UInt18 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt18, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 2, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 2);
        p.octets.octet3 = 0;

        return p.value;
    }
};

// SInt18_3 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt18_3, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= (int32_t)0xfffc0000;
        }

        return p.value;
    }
};

// SInt18_3 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt18_3, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= (int32_t)0xfffc0000;
        }

        return p.value;
    }
};

// UInt18_3 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt18_3, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0x3ffffu;

        return p.value;
    }
};

// UInt18_3 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt18_3, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        return p.value;
    }
};

// SInt18_4 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt18_4, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= (int32_t)0xfffc0000;
        }

        return p.value;
    }
};

// SInt18_4 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt18_4, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffff;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0x3ffff;

        if (p.value & 0x20000) {
            // sign extension
            p.value |= (int32_t)0xfffc0000;
        }

        return p.value;
    }
};

// UInt18_4 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt18_4, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0x3ffffu;

        return p.value;
    }
};

// UInt18_4 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt18_4, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0x3ffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0x3ffffu;

        return p.value;
    }
};

// SInt20 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt20, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, bit_offset, 4, p.octets.octet2);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 4);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);

        if (p.value & 0x80000) {
            // sign extension
            p.value |= (int32_t)0xfff00000;
        }

        return p.value;
    }
};

// SInt20 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt20, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 4, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 4);
        p.octets.octet3 = 0;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= (int32_t)0xfff00000;
        }

        return p.value;
    }
};

// UInt20 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt20, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_unaligned_write(buffer, bit_offset, 4, p.octets.octet2);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 4);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);

        return p.value;
    }
};

// UInt20 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt20, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet0);
        pcm_unaligned_write(buffer, bit_offset, 8, p.octets.octet1);
        pcm_unaligned_write(buffer, bit_offset, 4, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet1 = pcm_unaligned_read(buffer, bit_offset, 8);
        p.octets.octet2 = pcm_unaligned_read(buffer, bit_offset, 4);
        p.octets.octet3 = 0;

        return p.value;
    }
};

// SInt20_3 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt20_3, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= (int32_t)0xfff00000;
        }

        return p.value;
    }
};

// SInt20_3 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt20_3, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= (int32_t)0xfff00000;
        }

        return p.value;
    }
};

// UInt20_3 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt20_3, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xfffffu;

        return p.value;
    }
};

// UInt20_3 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt20_3, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = 0;

        // zeroise padding bits
        p.value &= 0xfffffu;

        return p.value;
    }
};

// SInt20_4 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt20_4, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= (int32_t)0xfff00000;
        }

        return p.value;
    }
};

// SInt20_4 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt20_4, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffff;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xfffff;

        if (p.value & 0x80000) {
            // sign extension
            p.value |= (int32_t)0xfff00000;
        }

        return p.value;
    }
};

// UInt20_4 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt20_4, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xfffffu;

        return p.value;
    }
};

// UInt20_4 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt20_4, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xfffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xfffffu;

        return p.value;
    }
};

// SInt24 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt24, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        if (p.value & 0x800000) {
            // sign extension
            p.value |= (int32_t)0xff000000;
        }

        return p.value;
    }
};

// SInt24 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt24, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = 0;

        if (p.value & 0x800000) {
            // sign extension
            p.value |= (int32_t)0xff000000;
        }

        return p.value;
    }
};

// UInt24 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt24, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = 0;
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt24 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt24, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = 0;

        return p.value;
    }
};

// SInt24_4 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt24_4, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffff;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xffffff;

        if (p.value & 0x800000) {
            // sign extension
            p.value |= (int32_t)0xff000000;
        }

        return p.value;
    }
};

// SInt24_4 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt24_4, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffff;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xffffff;

        if (p.value & 0x800000) {
            // sign extension
            p.value |= (int32_t)0xff000000;
        }

        return p.value;
    }
};

// UInt24_4 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt24_4, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffffu;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xffffffu;

        return p.value;
    }
};

// UInt24_4 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt24_4, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // zeroise padding bits
        p.value &= 0xffffffu;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        // zeroise padding bits
        p.value &= 0xffffffu;

        return p.value;
    }
};

// SInt32 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt32, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// SInt32 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt32, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int32_t arg) {
        // native-endian view of octets
        pcm_sample<int32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline int32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt32 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt32, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt32 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt32, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint32_t arg) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline uint32_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint32_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// SInt64 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt64, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int64_t arg) {
        // native-endian view of octets
        pcm_sample<int64_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet7);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet6);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet5);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet4);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline int64_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int64_t> p;

        // read in big-endian order
        p.octets.octet7 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet6 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet5 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet4 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// SInt64 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_SInt64, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, int64_t arg) {
        // native-endian view of octets
        pcm_sample<int64_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet4);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet5);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet6);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet7);
    }

    // Unpack next sample from buffer
    static inline int64_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<int64_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet4 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet5 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet6 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet7 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt64 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt64, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint64_t arg) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet7);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet6);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet5);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet4);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline uint64_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;

        // read in big-endian order
        p.octets.octet7 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet6 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet5 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet4 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// UInt64 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_UInt64, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, uint64_t arg) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet4);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet5);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet6);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet7);
    }

    // Unpack next sample from buffer
    static inline uint64_t unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<uint64_t> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet4 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet5 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet6 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet7 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// Float32 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_Float32, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, float arg) {
        // native-endian view of octets
        pcm_sample<float> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline float unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<float> p;

        // read in big-endian order
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// Float32 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_Float32, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, float arg) {
        // native-endian view of octets
        pcm_sample<float> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
    }

    // Unpack next sample from buffer
    static inline float unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<float> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// Float64 Big-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_Float64, PcmEndian_Big> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, double arg) {
        // native-endian view of octets
        pcm_sample<double> p;
        p.value = arg;

        // write in big-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet7);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet6);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet5);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet4);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
    }

    // Unpack next sample from buffer
    static inline double unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<double> p;

        // read in big-endian order
        p.octets.octet7 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet6 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet5 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet4 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

// Float64 Little-Endian packer / unpacker
template <> struct pcm_packer<PcmCode_Float64, PcmEndian_Little> {
    // Pack next sample to buffer
    static inline void pack(uint8_t* buffer, size_t& bit_offset, double arg) {
        // native-endian view of octets
        pcm_sample<double> p;
        p.value = arg;

        // write in little-endian order
        pcm_aligned_write(buffer, bit_offset, p.octets.octet0);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet1);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet2);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet3);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet4);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet5);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet6);
        pcm_aligned_write(buffer, bit_offset, p.octets.octet7);
    }

    // Unpack next sample from buffer
    static inline double unpack(const uint8_t* buffer, size_t& bit_offset) {
        // native-endian view of octets
        pcm_sample<double> p;

        // read in little-endian order
        p.octets.octet0 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet1 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet2 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet3 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet4 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet5 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet6 = pcm_aligned_read(buffer, bit_offset);
        p.octets.octet7 = pcm_aligned_read(buffer, bit_offset);

        return p.value;
    }
};

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
    case PcmEndian_Native:
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        return pcm_map_func<InCode, OutCode, InEndian, PcmEndian_Big>();
#else
        return pcm_map_func<InCode, OutCode, InEndian, PcmEndian_Little>();
#endif
    case PcmEndian_Big:
        return pcm_map_func<InCode, OutCode, InEndian, PcmEndian_Big>();
    case PcmEndian_Little:
        return pcm_map_func<InCode, OutCode, InEndian, PcmEndian_Little>();
    case PcmEndian_Max:
        break;
    }
    return NULL;
}

// Select mapping function
template <PcmCode InCode, PcmCode OutCode>
pcm_map_func_t pcm_map_func(PcmEndian in_endian, PcmEndian out_endian) {
    switch (in_endian) {
    case PcmEndian_Native:
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        return pcm_map_func<InCode, OutCode, PcmEndian_Big>(out_endian);
#else
        return pcm_map_func<InCode, OutCode, PcmEndian_Little>(out_endian);
#endif
    case PcmEndian_Big:
        return pcm_map_func<InCode, OutCode, PcmEndian_Big>(out_endian);
    case PcmEndian_Little:
        return pcm_map_func<InCode, OutCode, PcmEndian_Little>(out_endian);
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
    case PcmCode_SInt8:
        return pcm_map_func<InCode, PcmCode_SInt8>(in_endian, out_endian);
    case PcmCode_UInt8:
        return pcm_map_func<InCode, PcmCode_UInt8>(in_endian, out_endian);
    case PcmCode_SInt16:
        return pcm_map_func<InCode, PcmCode_SInt16>(in_endian, out_endian);
    case PcmCode_UInt16:
        return pcm_map_func<InCode, PcmCode_UInt16>(in_endian, out_endian);
    case PcmCode_SInt18:
        return pcm_map_func<InCode, PcmCode_SInt18>(in_endian, out_endian);
    case PcmCode_UInt18:
        return pcm_map_func<InCode, PcmCode_UInt18>(in_endian, out_endian);
    case PcmCode_SInt18_3:
        return pcm_map_func<InCode, PcmCode_SInt18_3>(in_endian, out_endian);
    case PcmCode_UInt18_3:
        return pcm_map_func<InCode, PcmCode_UInt18_3>(in_endian, out_endian);
    case PcmCode_SInt18_4:
        return pcm_map_func<InCode, PcmCode_SInt18_4>(in_endian, out_endian);
    case PcmCode_UInt18_4:
        return pcm_map_func<InCode, PcmCode_UInt18_4>(in_endian, out_endian);
    case PcmCode_SInt20:
        return pcm_map_func<InCode, PcmCode_SInt20>(in_endian, out_endian);
    case PcmCode_UInt20:
        return pcm_map_func<InCode, PcmCode_UInt20>(in_endian, out_endian);
    case PcmCode_SInt20_3:
        return pcm_map_func<InCode, PcmCode_SInt20_3>(in_endian, out_endian);
    case PcmCode_UInt20_3:
        return pcm_map_func<InCode, PcmCode_UInt20_3>(in_endian, out_endian);
    case PcmCode_SInt20_4:
        return pcm_map_func<InCode, PcmCode_SInt20_4>(in_endian, out_endian);
    case PcmCode_UInt20_4:
        return pcm_map_func<InCode, PcmCode_UInt20_4>(in_endian, out_endian);
    case PcmCode_SInt24:
        return pcm_map_func<InCode, PcmCode_SInt24>(in_endian, out_endian);
    case PcmCode_UInt24:
        return pcm_map_func<InCode, PcmCode_UInt24>(in_endian, out_endian);
    case PcmCode_SInt24_4:
        return pcm_map_func<InCode, PcmCode_SInt24_4>(in_endian, out_endian);
    case PcmCode_UInt24_4:
        return pcm_map_func<InCode, PcmCode_UInt24_4>(in_endian, out_endian);
    case PcmCode_SInt32:
        return pcm_map_func<InCode, PcmCode_SInt32>(in_endian, out_endian);
    case PcmCode_UInt32:
        return pcm_map_func<InCode, PcmCode_UInt32>(in_endian, out_endian);
    case PcmCode_SInt64:
        return pcm_map_func<InCode, PcmCode_SInt64>(in_endian, out_endian);
    case PcmCode_UInt64:
        return pcm_map_func<InCode, PcmCode_UInt64>(in_endian, out_endian);
    case PcmCode_Float32:
        return pcm_map_func<InCode, PcmCode_Float32>(in_endian, out_endian);
    case PcmCode_Float64:
        return pcm_map_func<InCode, PcmCode_Float64>(in_endian, out_endian);
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
    case PcmCode_SInt8:
        return pcm_map_func<PcmCode_SInt8>(out_code, in_endian, out_endian);
    case PcmCode_UInt8:
        return pcm_map_func<PcmCode_UInt8>(out_code, in_endian, out_endian);
    case PcmCode_SInt16:
        return pcm_map_func<PcmCode_SInt16>(out_code, in_endian, out_endian);
    case PcmCode_UInt16:
        return pcm_map_func<PcmCode_UInt16>(out_code, in_endian, out_endian);
    case PcmCode_SInt18:
        return pcm_map_func<PcmCode_SInt18>(out_code, in_endian, out_endian);
    case PcmCode_UInt18:
        return pcm_map_func<PcmCode_UInt18>(out_code, in_endian, out_endian);
    case PcmCode_SInt18_3:
        return pcm_map_func<PcmCode_SInt18_3>(out_code, in_endian, out_endian);
    case PcmCode_UInt18_3:
        return pcm_map_func<PcmCode_UInt18_3>(out_code, in_endian, out_endian);
    case PcmCode_SInt18_4:
        return pcm_map_func<PcmCode_SInt18_4>(out_code, in_endian, out_endian);
    case PcmCode_UInt18_4:
        return pcm_map_func<PcmCode_UInt18_4>(out_code, in_endian, out_endian);
    case PcmCode_SInt20:
        return pcm_map_func<PcmCode_SInt20>(out_code, in_endian, out_endian);
    case PcmCode_UInt20:
        return pcm_map_func<PcmCode_UInt20>(out_code, in_endian, out_endian);
    case PcmCode_SInt20_3:
        return pcm_map_func<PcmCode_SInt20_3>(out_code, in_endian, out_endian);
    case PcmCode_UInt20_3:
        return pcm_map_func<PcmCode_UInt20_3>(out_code, in_endian, out_endian);
    case PcmCode_SInt20_4:
        return pcm_map_func<PcmCode_SInt20_4>(out_code, in_endian, out_endian);
    case PcmCode_UInt20_4:
        return pcm_map_func<PcmCode_UInt20_4>(out_code, in_endian, out_endian);
    case PcmCode_SInt24:
        return pcm_map_func<PcmCode_SInt24>(out_code, in_endian, out_endian);
    case PcmCode_UInt24:
        return pcm_map_func<PcmCode_UInt24>(out_code, in_endian, out_endian);
    case PcmCode_SInt24_4:
        return pcm_map_func<PcmCode_SInt24_4>(out_code, in_endian, out_endian);
    case PcmCode_UInt24_4:
        return pcm_map_func<PcmCode_UInt24_4>(out_code, in_endian, out_endian);
    case PcmCode_SInt32:
        return pcm_map_func<PcmCode_SInt32>(out_code, in_endian, out_endian);
    case PcmCode_UInt32:
        return pcm_map_func<PcmCode_UInt32>(out_code, in_endian, out_endian);
    case PcmCode_SInt64:
        return pcm_map_func<PcmCode_SInt64>(out_code, in_endian, out_endian);
    case PcmCode_UInt64:
        return pcm_map_func<PcmCode_UInt64>(out_code, in_endian, out_endian);
    case PcmCode_Float32:
        return pcm_map_func<PcmCode_Float32>(out_code, in_endian, out_endian);
    case PcmCode_Float64:
        return pcm_map_func<PcmCode_Float64>(out_code, in_endian, out_endian);
    case PcmCode_Max:
        break;
    }
    return NULL;
}

// Get number of meaningful bits per sample
inline size_t pcm_bit_depth(PcmCode code) {
    switch (code) {
    case PcmCode_SInt8:
        return 8;
    case PcmCode_UInt8:
        return 8;
    case PcmCode_SInt16:
        return 16;
    case PcmCode_UInt16:
        return 16;
    case PcmCode_SInt18:
        return 18;
    case PcmCode_UInt18:
        return 18;
    case PcmCode_SInt18_3:
        return 18;
    case PcmCode_UInt18_3:
        return 18;
    case PcmCode_SInt18_4:
        return 18;
    case PcmCode_UInt18_4:
        return 18;
    case PcmCode_SInt20:
        return 20;
    case PcmCode_UInt20:
        return 20;
    case PcmCode_SInt20_3:
        return 20;
    case PcmCode_UInt20_3:
        return 20;
    case PcmCode_SInt20_4:
        return 20;
    case PcmCode_UInt20_4:
        return 20;
    case PcmCode_SInt24:
        return 24;
    case PcmCode_UInt24:
        return 24;
    case PcmCode_SInt24_4:
        return 24;
    case PcmCode_UInt24_4:
        return 24;
    case PcmCode_SInt32:
        return 32;
    case PcmCode_UInt32:
        return 32;
    case PcmCode_SInt64:
        return 64;
    case PcmCode_UInt64:
        return 64;
    case PcmCode_Float32:
        return 32;
    case PcmCode_Float64:
        return 64;
    case PcmCode_Max:
        break;
    }
    return 0;
}

// Get number of total bits per sample
inline size_t pcm_bit_width(PcmCode code) {
    switch (code) {
    case PcmCode_SInt8:
        return 8;
    case PcmCode_UInt8:
        return 8;
    case PcmCode_SInt16:
        return 16;
    case PcmCode_UInt16:
        return 16;
    case PcmCode_SInt18:
        return 18;
    case PcmCode_UInt18:
        return 18;
    case PcmCode_SInt18_3:
        return 24;
    case PcmCode_UInt18_3:
        return 24;
    case PcmCode_SInt18_4:
        return 32;
    case PcmCode_UInt18_4:
        return 32;
    case PcmCode_SInt20:
        return 20;
    case PcmCode_UInt20:
        return 20;
    case PcmCode_SInt20_3:
        return 24;
    case PcmCode_UInt20_3:
        return 24;
    case PcmCode_SInt20_4:
        return 32;
    case PcmCode_UInt20_4:
        return 32;
    case PcmCode_SInt24:
        return 24;
    case PcmCode_UInt24:
        return 24;
    case PcmCode_SInt24_4:
        return 32;
    case PcmCode_UInt24_4:
        return 32;
    case PcmCode_SInt32:
        return 32;
    case PcmCode_UInt32:
        return 32;
    case PcmCode_SInt64:
        return 64;
    case PcmCode_UInt64:
        return 64;
    case PcmCode_Float32:
        return 32;
    case PcmCode_Float64:
        return 64;
    case PcmCode_Max:
        break;
    }
    return 0;
}

// Check if code is integer
inline size_t pcm_is_integer(PcmCode code) {
    switch (code) {
    case PcmCode_SInt8:
        return true;
    case PcmCode_UInt8:
        return true;
    case PcmCode_SInt16:
        return true;
    case PcmCode_UInt16:
        return true;
    case PcmCode_SInt18:
        return true;
    case PcmCode_UInt18:
        return true;
    case PcmCode_SInt18_3:
        return true;
    case PcmCode_UInt18_3:
        return true;
    case PcmCode_SInt18_4:
        return true;
    case PcmCode_UInt18_4:
        return true;
    case PcmCode_SInt20:
        return true;
    case PcmCode_UInt20:
        return true;
    case PcmCode_SInt20_3:
        return true;
    case PcmCode_UInt20_3:
        return true;
    case PcmCode_SInt20_4:
        return true;
    case PcmCode_UInt20_4:
        return true;
    case PcmCode_SInt24:
        return true;
    case PcmCode_UInt24:
        return true;
    case PcmCode_SInt24_4:
        return true;
    case PcmCode_UInt24_4:
        return true;
    case PcmCode_SInt32:
        return true;
    case PcmCode_UInt32:
        return true;
    case PcmCode_SInt64:
        return true;
    case PcmCode_UInt64:
        return true;
    case PcmCode_Float32:
        return false;
    case PcmCode_Float64:
        return false;
    case PcmCode_Max:
        break;
    }
    return false;
}

// Check if code is signed
inline size_t pcm_is_signed(PcmCode code) {
    switch (code) {
    case PcmCode_SInt8:
        return true;
    case PcmCode_UInt8:
        return false;
    case PcmCode_SInt16:
        return true;
    case PcmCode_UInt16:
        return false;
    case PcmCode_SInt18:
        return true;
    case PcmCode_UInt18:
        return false;
    case PcmCode_SInt18_3:
        return true;
    case PcmCode_UInt18_3:
        return false;
    case PcmCode_SInt18_4:
        return true;
    case PcmCode_UInt18_4:
        return false;
    case PcmCode_SInt20:
        return true;
    case PcmCode_UInt20:
        return false;
    case PcmCode_SInt20_3:
        return true;
    case PcmCode_UInt20_3:
        return false;
    case PcmCode_SInt20_4:
        return true;
    case PcmCode_UInt20_4:
        return false;
    case PcmCode_SInt24:
        return true;
    case PcmCode_UInt24:
        return false;
    case PcmCode_SInt24_4:
        return true;
    case PcmCode_UInt24_4:
        return false;
    case PcmCode_SInt32:
        return true;
    case PcmCode_UInt32:
        return false;
    case PcmCode_SInt64:
        return true;
    case PcmCode_UInt64:
        return false;
    case PcmCode_Float32:
        return true;
    case PcmCode_Float64:
        return true;
    case PcmCode_Max:
        break;
    }
    return false;
}

// Code and endian to string
inline const char* pcm_to_str(PcmCode code, PcmEndian endian) {
    switch (code) {
    case PcmCode_SInt8:
        switch (endian) {
        case PcmEndian_Native:
            return "s8";
        case PcmEndian_Big:
            return "s8_be";
        case PcmEndian_Little:
            return "s8_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt8:
        switch (endian) {
        case PcmEndian_Native:
            return "u8";
        case PcmEndian_Big:
            return "u8_be";
        case PcmEndian_Little:
            return "u8_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt16:
        switch (endian) {
        case PcmEndian_Native:
            return "s16";
        case PcmEndian_Big:
            return "s16_be";
        case PcmEndian_Little:
            return "s16_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt16:
        switch (endian) {
        case PcmEndian_Native:
            return "u16";
        case PcmEndian_Big:
            return "u16_be";
        case PcmEndian_Little:
            return "u16_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt18:
        switch (endian) {
        case PcmEndian_Native:
            return "s18";
        case PcmEndian_Big:
            return "s18_be";
        case PcmEndian_Little:
            return "s18_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt18:
        switch (endian) {
        case PcmEndian_Native:
            return "u18";
        case PcmEndian_Big:
            return "u18_be";
        case PcmEndian_Little:
            return "u18_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt18_3:
        switch (endian) {
        case PcmEndian_Native:
            return "s18_3";
        case PcmEndian_Big:
            return "s18_3be";
        case PcmEndian_Little:
            return "s18_3le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt18_3:
        switch (endian) {
        case PcmEndian_Native:
            return "u18_3";
        case PcmEndian_Big:
            return "u18_3be";
        case PcmEndian_Little:
            return "u18_3le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt18_4:
        switch (endian) {
        case PcmEndian_Native:
            return "s18_4";
        case PcmEndian_Big:
            return "s18_4be";
        case PcmEndian_Little:
            return "s18_4le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt18_4:
        switch (endian) {
        case PcmEndian_Native:
            return "u18_4";
        case PcmEndian_Big:
            return "u18_4be";
        case PcmEndian_Little:
            return "u18_4le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt20:
        switch (endian) {
        case PcmEndian_Native:
            return "s20";
        case PcmEndian_Big:
            return "s20_be";
        case PcmEndian_Little:
            return "s20_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt20:
        switch (endian) {
        case PcmEndian_Native:
            return "u20";
        case PcmEndian_Big:
            return "u20_be";
        case PcmEndian_Little:
            return "u20_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt20_3:
        switch (endian) {
        case PcmEndian_Native:
            return "s20_3";
        case PcmEndian_Big:
            return "s20_3be";
        case PcmEndian_Little:
            return "s20_3le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt20_3:
        switch (endian) {
        case PcmEndian_Native:
            return "u20_3";
        case PcmEndian_Big:
            return "u20_3be";
        case PcmEndian_Little:
            return "u20_3le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt20_4:
        switch (endian) {
        case PcmEndian_Native:
            return "s20_4";
        case PcmEndian_Big:
            return "s20_4be";
        case PcmEndian_Little:
            return "s20_4le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt20_4:
        switch (endian) {
        case PcmEndian_Native:
            return "u20_4";
        case PcmEndian_Big:
            return "u20_4be";
        case PcmEndian_Little:
            return "u20_4le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt24:
        switch (endian) {
        case PcmEndian_Native:
            return "s24";
        case PcmEndian_Big:
            return "s24_be";
        case PcmEndian_Little:
            return "s24_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt24:
        switch (endian) {
        case PcmEndian_Native:
            return "u24";
        case PcmEndian_Big:
            return "u24_be";
        case PcmEndian_Little:
            return "u24_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt24_4:
        switch (endian) {
        case PcmEndian_Native:
            return "s24_4";
        case PcmEndian_Big:
            return "s24_4be";
        case PcmEndian_Little:
            return "s24_4le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt24_4:
        switch (endian) {
        case PcmEndian_Native:
            return "u24_4";
        case PcmEndian_Big:
            return "u24_4be";
        case PcmEndian_Little:
            return "u24_4le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt32:
        switch (endian) {
        case PcmEndian_Native:
            return "s32";
        case PcmEndian_Big:
            return "s32_be";
        case PcmEndian_Little:
            return "s32_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt32:
        switch (endian) {
        case PcmEndian_Native:
            return "u32";
        case PcmEndian_Big:
            return "u32_be";
        case PcmEndian_Little:
            return "u32_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_SInt64:
        switch (endian) {
        case PcmEndian_Native:
            return "s64";
        case PcmEndian_Big:
            return "s64_be";
        case PcmEndian_Little:
            return "s64_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_UInt64:
        switch (endian) {
        case PcmEndian_Native:
            return "u64";
        case PcmEndian_Big:
            return "u64_be";
        case PcmEndian_Little:
            return "u64_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_Float32:
        switch (endian) {
        case PcmEndian_Native:
            return "f32";
        case PcmEndian_Big:
            return "f32_be";
        case PcmEndian_Little:
            return "f32_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_Float64:
        switch (endian) {
        case PcmEndian_Native:
            return "f64";
        case PcmEndian_Big:
            return "f64_be";
        case PcmEndian_Little:
            return "f64_le";
        case PcmEndian_Max:
            break;
        }
        break;
    case PcmCode_Max:
        break;
    }
    return NULL;
}

// Code and endian from string
inline bool pcm_from_str(const char* str, PcmCode& code, PcmEndian& endian) {
    if (str[0] == 'f') {
        if (str[1] == '3') {
            if (str[2] == '2') {
                if (strcmp(str, "f32") == 0) {
                    code = PcmCode_Float32;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "f32_be") == 0) {
                    code = PcmCode_Float32;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "f32_le") == 0) {
                    code = PcmCode_Float32;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '6') {
            if (str[2] == '4') {
                if (strcmp(str, "f64") == 0) {
                    code = PcmCode_Float64;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "f64_be") == 0) {
                    code = PcmCode_Float64;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "f64_le") == 0) {
                    code = PcmCode_Float64;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        return false;
    }
    if (str[0] == 's') {
        if (str[1] == '1') {
            if (str[2] == '6') {
                if (strcmp(str, "s16") == 0) {
                    code = PcmCode_SInt16;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s16_be") == 0) {
                    code = PcmCode_SInt16;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s16_le") == 0) {
                    code = PcmCode_SInt16;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            if (str[2] == '8') {
                if (strcmp(str, "s18") == 0) {
                    code = PcmCode_SInt18;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s18_be") == 0) {
                    code = PcmCode_SInt18;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s18_le") == 0) {
                    code = PcmCode_SInt18;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "s18_3") == 0) {
                    code = PcmCode_SInt18_3;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s18_3be") == 0) {
                    code = PcmCode_SInt18_3;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s18_3le") == 0) {
                    code = PcmCode_SInt18_3;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "s18_4") == 0) {
                    code = PcmCode_SInt18_4;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s18_4be") == 0) {
                    code = PcmCode_SInt18_4;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s18_4le") == 0) {
                    code = PcmCode_SInt18_4;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '2') {
            if (str[2] == '0') {
                if (strcmp(str, "s20") == 0) {
                    code = PcmCode_SInt20;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s20_be") == 0) {
                    code = PcmCode_SInt20;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s20_le") == 0) {
                    code = PcmCode_SInt20;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "s20_3") == 0) {
                    code = PcmCode_SInt20_3;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s20_3be") == 0) {
                    code = PcmCode_SInt20_3;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s20_3le") == 0) {
                    code = PcmCode_SInt20_3;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "s20_4") == 0) {
                    code = PcmCode_SInt20_4;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s20_4be") == 0) {
                    code = PcmCode_SInt20_4;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s20_4le") == 0) {
                    code = PcmCode_SInt20_4;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            if (str[2] == '4') {
                if (strcmp(str, "s24") == 0) {
                    code = PcmCode_SInt24;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s24_be") == 0) {
                    code = PcmCode_SInt24;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s24_le") == 0) {
                    code = PcmCode_SInt24;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "s24_4") == 0) {
                    code = PcmCode_SInt24_4;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s24_4be") == 0) {
                    code = PcmCode_SInt24_4;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s24_4le") == 0) {
                    code = PcmCode_SInt24_4;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '3') {
            if (str[2] == '2') {
                if (strcmp(str, "s32") == 0) {
                    code = PcmCode_SInt32;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s32_be") == 0) {
                    code = PcmCode_SInt32;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s32_le") == 0) {
                    code = PcmCode_SInt32;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '6') {
            if (str[2] == '4') {
                if (strcmp(str, "s64") == 0) {
                    code = PcmCode_SInt64;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "s64_be") == 0) {
                    code = PcmCode_SInt64;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "s64_le") == 0) {
                    code = PcmCode_SInt64;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '8') {
            if (strcmp(str, "s8") == 0) {
                code = PcmCode_SInt8;
                endian = PcmEndian_Native;
                return true;
            }
            if (strcmp(str, "s8_be") == 0) {
                code = PcmCode_SInt8;
                endian = PcmEndian_Big;
                return true;
            }
            if (strcmp(str, "s8_le") == 0) {
                code = PcmCode_SInt8;
                endian = PcmEndian_Little;
                return true;
            }
            return false;
        }
        return false;
    }
    if (str[0] == 'u') {
        if (str[1] == '1') {
            if (str[2] == '6') {
                if (strcmp(str, "u16") == 0) {
                    code = PcmCode_UInt16;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u16_be") == 0) {
                    code = PcmCode_UInt16;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u16_le") == 0) {
                    code = PcmCode_UInt16;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            if (str[2] == '8') {
                if (strcmp(str, "u18") == 0) {
                    code = PcmCode_UInt18;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u18_be") == 0) {
                    code = PcmCode_UInt18;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u18_le") == 0) {
                    code = PcmCode_UInt18;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "u18_3") == 0) {
                    code = PcmCode_UInt18_3;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u18_3be") == 0) {
                    code = PcmCode_UInt18_3;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u18_3le") == 0) {
                    code = PcmCode_UInt18_3;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "u18_4") == 0) {
                    code = PcmCode_UInt18_4;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u18_4be") == 0) {
                    code = PcmCode_UInt18_4;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u18_4le") == 0) {
                    code = PcmCode_UInt18_4;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '2') {
            if (str[2] == '0') {
                if (strcmp(str, "u20") == 0) {
                    code = PcmCode_UInt20;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u20_be") == 0) {
                    code = PcmCode_UInt20;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u20_le") == 0) {
                    code = PcmCode_UInt20;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "u20_3") == 0) {
                    code = PcmCode_UInt20_3;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u20_3be") == 0) {
                    code = PcmCode_UInt20_3;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u20_3le") == 0) {
                    code = PcmCode_UInt20_3;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "u20_4") == 0) {
                    code = PcmCode_UInt20_4;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u20_4be") == 0) {
                    code = PcmCode_UInt20_4;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u20_4le") == 0) {
                    code = PcmCode_UInt20_4;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            if (str[2] == '4') {
                if (strcmp(str, "u24") == 0) {
                    code = PcmCode_UInt24;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u24_be") == 0) {
                    code = PcmCode_UInt24;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u24_le") == 0) {
                    code = PcmCode_UInt24;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
                if (strcmp(str, "u24_4") == 0) {
                    code = PcmCode_UInt24_4;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u24_4be") == 0) {
                    code = PcmCode_UInt24_4;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u24_4le") == 0) {
                    code = PcmCode_UInt24_4;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '3') {
            if (str[2] == '2') {
                if (strcmp(str, "u32") == 0) {
                    code = PcmCode_UInt32;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u32_be") == 0) {
                    code = PcmCode_UInt32;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u32_le") == 0) {
                    code = PcmCode_UInt32;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '6') {
            if (str[2] == '4') {
                if (strcmp(str, "u64") == 0) {
                    code = PcmCode_UInt64;
                    endian = PcmEndian_Native;
                    return true;
                }
                if (strcmp(str, "u64_be") == 0) {
                    code = PcmCode_UInt64;
                    endian = PcmEndian_Big;
                    return true;
                }
                if (strcmp(str, "u64_le") == 0) {
                    code = PcmCode_UInt64;
                    endian = PcmEndian_Little;
                    return true;
                }
                return false;
            }
            return false;
        }
        if (str[1] == '8') {
            if (strcmp(str, "u8") == 0) {
                code = PcmCode_UInt8;
                endian = PcmEndian_Native;
                return true;
            }
            if (strcmp(str, "u8_be") == 0) {
                code = PcmCode_UInt8;
                endian = PcmEndian_Big;
                return true;
            }
            if (strcmp(str, "u8_le") == 0) {
                code = PcmCode_UInt8;
                endian = PcmEndian_Little;
                return true;
            }
            return false;
        }
        return false;
    }
    return false;
}

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_FUNCS_H_
