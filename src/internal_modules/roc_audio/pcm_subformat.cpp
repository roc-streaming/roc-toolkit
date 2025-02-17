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
    PcmCode_SInt8,
    PcmCode_UInt8,
    PcmCode_SInt16,
    PcmCode_UInt16,
    PcmCode_SInt18,
    PcmCode_UInt18,
    PcmCode_SInt18_3,
    PcmCode_UInt18_3,
    PcmCode_SInt18_4,
    PcmCode_UInt18_4,
    PcmCode_SInt20,
    PcmCode_UInt20,
    PcmCode_SInt20_3,
    PcmCode_UInt20_3,
    PcmCode_SInt20_4,
    PcmCode_UInt20_4,
    PcmCode_SInt24,
    PcmCode_UInt24,
    PcmCode_SInt24_4,
    PcmCode_UInt24_4,
    PcmCode_SInt32,
    PcmCode_UInt32,
    PcmCode_SInt64,
    PcmCode_UInt64,
    PcmCode_Float32,
    PcmCode_Float64,
};

// PCM endians.
enum PcmEndian {
    PcmEndian_Big,
    PcmEndian_Little,
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    PcmEndian_Default = PcmEndian_Big,
#else
    PcmEndian_Default = PcmEndian_Little,
#endif
};

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

// Convert between unpacked codes
template <PcmCode InCode, PcmCode OutCode> struct pcm_code_converter;

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

// N-byte native-endian sample
template <class T> struct pcm_sample;

// int8_t native-endian sample
template <> struct pcm_sample<int8_t> {
    union {
        int8_t value;
        ROC_ATTR_PACKED_BEGIN struct {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
            uint8_t octet0;
#else
            uint8_t octet0;
#endif
        } ROC_ATTR_PACKED_END octets;
    };
};

// uint8_t native-endian sample
template <> struct pcm_sample<uint8_t> {
    union {
        uint8_t value;
        ROC_ATTR_PACKED_BEGIN struct {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
            uint8_t octet0;
#else
            uint8_t octet0;
#endif
        } ROC_ATTR_PACKED_END octets;
    };
};

// int16_t native-endian sample
template <> struct pcm_sample<int16_t> {
    union {
        int16_t value;
        ROC_ATTR_PACKED_BEGIN struct {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
            uint8_t octet1;
            uint8_t octet0;
#else
            uint8_t octet0;
            uint8_t octet1;
#endif
        } ROC_ATTR_PACKED_END octets;
    };
};

// uint16_t native-endian sample
template <> struct pcm_sample<uint16_t> {
    union {
        uint16_t value;
        ROC_ATTR_PACKED_BEGIN struct {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
            uint8_t octet1;
            uint8_t octet0;
#else
            uint8_t octet0;
            uint8_t octet1;
#endif
        } ROC_ATTR_PACKED_END octets;
    };
};

// int32_t native-endian sample
template <> struct pcm_sample<int32_t> {
    union {
        int32_t value;
        ROC_ATTR_PACKED_BEGIN struct {
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
        } ROC_ATTR_PACKED_END octets;
    };
};

// uint32_t native-endian sample
template <> struct pcm_sample<uint32_t> {
    union {
        uint32_t value;
        ROC_ATTR_PACKED_BEGIN struct {
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
        } ROC_ATTR_PACKED_END octets;
    };
};

// int64_t native-endian sample
template <> struct pcm_sample<int64_t> {
    union {
        int64_t value;
        ROC_ATTR_PACKED_BEGIN struct {
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
        } ROC_ATTR_PACKED_END octets;
    };
};

// uint64_t native-endian sample
template <> struct pcm_sample<uint64_t> {
    union {
        uint64_t value;
        ROC_ATTR_PACKED_BEGIN struct {
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
        } ROC_ATTR_PACKED_END octets;
    };
};

// float native-endian sample
template <> struct pcm_sample<float> {
    union {
        float value;
        ROC_ATTR_PACKED_BEGIN struct {
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
        } ROC_ATTR_PACKED_END octets;
    };
};

// double native-endian sample
template <> struct pcm_sample<double> {
    union {
        double value;
        ROC_ATTR_PACKED_BEGIN struct {
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
        } ROC_ATTR_PACKED_END octets;
    };
};

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
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    case PcmSubformat_Float32:
    case PcmSubformat_Float32_Be:
#else
    case PcmSubformat_Float32:
    case PcmSubformat_Float32_Le:
#endif
        return &pcm_mapper<InCode, InEndian, PcmCode_Float32, PcmEndian_Default>::map;
    default:
        break;
    }
    return NULL;
}

// Select mapping function
template <PcmCode OutCode, PcmEndian OutEndian>
PcmMapFn pcm_map_from_raw(PcmSubformat raw_format) {
    switch (raw_format) {
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    case PcmSubformat_Float32:
    case PcmSubformat_Float32_Be:
#else
    case PcmSubformat_Float32:
    case PcmSubformat_Float32_Le:
#endif
        return &pcm_mapper<PcmCode_Float32, PcmEndian_Default, OutCode, OutEndian>::map;
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
    case PcmSubformat_SInt8:
        return pcm_map_to_raw<PcmCode_SInt8, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt8_Be:
        return pcm_map_to_raw<PcmCode_SInt8, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt8_Le:
        return pcm_map_to_raw<PcmCode_SInt8, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt8:
        return pcm_map_to_raw<PcmCode_UInt8, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt8_Be:
        return pcm_map_to_raw<PcmCode_UInt8, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt8_Le:
        return pcm_map_to_raw<PcmCode_UInt8, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt16:
        return pcm_map_to_raw<PcmCode_SInt16, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt16_Be:
        return pcm_map_to_raw<PcmCode_SInt16, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt16_Le:
        return pcm_map_to_raw<PcmCode_SInt16, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt16:
        return pcm_map_to_raw<PcmCode_UInt16, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt16_Be:
        return pcm_map_to_raw<PcmCode_UInt16, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt16_Le:
        return pcm_map_to_raw<PcmCode_UInt16, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt18:
        return pcm_map_to_raw<PcmCode_SInt18, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt18_Be:
        return pcm_map_to_raw<PcmCode_SInt18, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt18_Le:
        return pcm_map_to_raw<PcmCode_SInt18, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt18:
        return pcm_map_to_raw<PcmCode_UInt18, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt18_Be:
        return pcm_map_to_raw<PcmCode_UInt18, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt18_Le:
        return pcm_map_to_raw<PcmCode_UInt18, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt18_3:
        return pcm_map_to_raw<PcmCode_SInt18_3, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt18_3_Be:
        return pcm_map_to_raw<PcmCode_SInt18_3, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt18_3_Le:
        return pcm_map_to_raw<PcmCode_SInt18_3, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt18_3:
        return pcm_map_to_raw<PcmCode_UInt18_3, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt18_3_Be:
        return pcm_map_to_raw<PcmCode_UInt18_3, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt18_3_Le:
        return pcm_map_to_raw<PcmCode_UInt18_3, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt18_4:
        return pcm_map_to_raw<PcmCode_SInt18_4, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt18_4_Be:
        return pcm_map_to_raw<PcmCode_SInt18_4, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt18_4_Le:
        return pcm_map_to_raw<PcmCode_SInt18_4, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt18_4:
        return pcm_map_to_raw<PcmCode_UInt18_4, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt18_4_Be:
        return pcm_map_to_raw<PcmCode_UInt18_4, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt18_4_Le:
        return pcm_map_to_raw<PcmCode_UInt18_4, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt20:
        return pcm_map_to_raw<PcmCode_SInt20, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt20_Be:
        return pcm_map_to_raw<PcmCode_SInt20, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt20_Le:
        return pcm_map_to_raw<PcmCode_SInt20, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt20:
        return pcm_map_to_raw<PcmCode_UInt20, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt20_Be:
        return pcm_map_to_raw<PcmCode_UInt20, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt20_Le:
        return pcm_map_to_raw<PcmCode_UInt20, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt20_3:
        return pcm_map_to_raw<PcmCode_SInt20_3, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt20_3_Be:
        return pcm_map_to_raw<PcmCode_SInt20_3, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt20_3_Le:
        return pcm_map_to_raw<PcmCode_SInt20_3, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt20_3:
        return pcm_map_to_raw<PcmCode_UInt20_3, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt20_3_Be:
        return pcm_map_to_raw<PcmCode_UInt20_3, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt20_3_Le:
        return pcm_map_to_raw<PcmCode_UInt20_3, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt20_4:
        return pcm_map_to_raw<PcmCode_SInt20_4, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt20_4_Be:
        return pcm_map_to_raw<PcmCode_SInt20_4, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt20_4_Le:
        return pcm_map_to_raw<PcmCode_SInt20_4, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt20_4:
        return pcm_map_to_raw<PcmCode_UInt20_4, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt20_4_Be:
        return pcm_map_to_raw<PcmCode_UInt20_4, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt20_4_Le:
        return pcm_map_to_raw<PcmCode_UInt20_4, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt24:
        return pcm_map_to_raw<PcmCode_SInt24, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt24_Be:
        return pcm_map_to_raw<PcmCode_SInt24, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt24_Le:
        return pcm_map_to_raw<PcmCode_SInt24, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt24:
        return pcm_map_to_raw<PcmCode_UInt24, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt24_Be:
        return pcm_map_to_raw<PcmCode_UInt24, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt24_Le:
        return pcm_map_to_raw<PcmCode_UInt24, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt24_4:
        return pcm_map_to_raw<PcmCode_SInt24_4, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt24_4_Be:
        return pcm_map_to_raw<PcmCode_SInt24_4, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt24_4_Le:
        return pcm_map_to_raw<PcmCode_SInt24_4, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt24_4:
        return pcm_map_to_raw<PcmCode_UInt24_4, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt24_4_Be:
        return pcm_map_to_raw<PcmCode_UInt24_4, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt24_4_Le:
        return pcm_map_to_raw<PcmCode_UInt24_4, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt32:
        return pcm_map_to_raw<PcmCode_SInt32, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt32_Be:
        return pcm_map_to_raw<PcmCode_SInt32, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt32_Le:
        return pcm_map_to_raw<PcmCode_SInt32, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt32:
        return pcm_map_to_raw<PcmCode_UInt32, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt32_Be:
        return pcm_map_to_raw<PcmCode_UInt32, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt32_Le:
        return pcm_map_to_raw<PcmCode_UInt32, PcmEndian_Little>(out_format);
    case PcmSubformat_SInt64:
        return pcm_map_to_raw<PcmCode_SInt64, PcmEndian_Default>(out_format);
    case PcmSubformat_SInt64_Be:
        return pcm_map_to_raw<PcmCode_SInt64, PcmEndian_Big>(out_format);
    case PcmSubformat_SInt64_Le:
        return pcm_map_to_raw<PcmCode_SInt64, PcmEndian_Little>(out_format);
    case PcmSubformat_UInt64:
        return pcm_map_to_raw<PcmCode_UInt64, PcmEndian_Default>(out_format);
    case PcmSubformat_UInt64_Be:
        return pcm_map_to_raw<PcmCode_UInt64, PcmEndian_Big>(out_format);
    case PcmSubformat_UInt64_Le:
        return pcm_map_to_raw<PcmCode_UInt64, PcmEndian_Little>(out_format);
#if ROC_CPU_ENDIAN != ROC_CPU_BE
    case PcmSubformat_Float32_Be:
        return pcm_map_to_raw<PcmCode_Float32, PcmEndian_Big>(out_format);
#else
    case PcmSubformat_Float32_Le:
        return pcm_map_to_raw<PcmCode_Float32, PcmEndian_Little>(out_format);
#endif
    case PcmSubformat_Float64:
        return pcm_map_to_raw<PcmCode_Float64, PcmEndian_Default>(out_format);
    case PcmSubformat_Float64_Be:
        return pcm_map_to_raw<PcmCode_Float64, PcmEndian_Big>(out_format);
    case PcmSubformat_Float64_Le:
        return pcm_map_to_raw<PcmCode_Float64, PcmEndian_Little>(out_format);
    default:
        break;
    }

    // raw to non-raw
    switch (out_format) {
    case PcmSubformat_SInt8:
        return pcm_map_from_raw<PcmCode_SInt8, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt8_Be:
        return pcm_map_from_raw<PcmCode_SInt8, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt8_Le:
        return pcm_map_from_raw<PcmCode_SInt8, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt8:
        return pcm_map_from_raw<PcmCode_UInt8, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt8_Be:
        return pcm_map_from_raw<PcmCode_UInt8, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt8_Le:
        return pcm_map_from_raw<PcmCode_UInt8, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt16:
        return pcm_map_from_raw<PcmCode_SInt16, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt16_Be:
        return pcm_map_from_raw<PcmCode_SInt16, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt16_Le:
        return pcm_map_from_raw<PcmCode_SInt16, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt16:
        return pcm_map_from_raw<PcmCode_UInt16, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt16_Be:
        return pcm_map_from_raw<PcmCode_UInt16, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt16_Le:
        return pcm_map_from_raw<PcmCode_UInt16, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt18:
        return pcm_map_from_raw<PcmCode_SInt18, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt18_Be:
        return pcm_map_from_raw<PcmCode_SInt18, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt18_Le:
        return pcm_map_from_raw<PcmCode_SInt18, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt18:
        return pcm_map_from_raw<PcmCode_UInt18, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt18_Be:
        return pcm_map_from_raw<PcmCode_UInt18, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt18_Le:
        return pcm_map_from_raw<PcmCode_UInt18, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt18_3:
        return pcm_map_from_raw<PcmCode_SInt18_3, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt18_3_Be:
        return pcm_map_from_raw<PcmCode_SInt18_3, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt18_3_Le:
        return pcm_map_from_raw<PcmCode_SInt18_3, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt18_3:
        return pcm_map_from_raw<PcmCode_UInt18_3, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt18_3_Be:
        return pcm_map_from_raw<PcmCode_UInt18_3, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt18_3_Le:
        return pcm_map_from_raw<PcmCode_UInt18_3, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt18_4:
        return pcm_map_from_raw<PcmCode_SInt18_4, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt18_4_Be:
        return pcm_map_from_raw<PcmCode_SInt18_4, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt18_4_Le:
        return pcm_map_from_raw<PcmCode_SInt18_4, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt18_4:
        return pcm_map_from_raw<PcmCode_UInt18_4, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt18_4_Be:
        return pcm_map_from_raw<PcmCode_UInt18_4, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt18_4_Le:
        return pcm_map_from_raw<PcmCode_UInt18_4, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt20:
        return pcm_map_from_raw<PcmCode_SInt20, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt20_Be:
        return pcm_map_from_raw<PcmCode_SInt20, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt20_Le:
        return pcm_map_from_raw<PcmCode_SInt20, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt20:
        return pcm_map_from_raw<PcmCode_UInt20, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt20_Be:
        return pcm_map_from_raw<PcmCode_UInt20, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt20_Le:
        return pcm_map_from_raw<PcmCode_UInt20, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt20_3:
        return pcm_map_from_raw<PcmCode_SInt20_3, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt20_3_Be:
        return pcm_map_from_raw<PcmCode_SInt20_3, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt20_3_Le:
        return pcm_map_from_raw<PcmCode_SInt20_3, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt20_3:
        return pcm_map_from_raw<PcmCode_UInt20_3, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt20_3_Be:
        return pcm_map_from_raw<PcmCode_UInt20_3, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt20_3_Le:
        return pcm_map_from_raw<PcmCode_UInt20_3, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt20_4:
        return pcm_map_from_raw<PcmCode_SInt20_4, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt20_4_Be:
        return pcm_map_from_raw<PcmCode_SInt20_4, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt20_4_Le:
        return pcm_map_from_raw<PcmCode_SInt20_4, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt20_4:
        return pcm_map_from_raw<PcmCode_UInt20_4, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt20_4_Be:
        return pcm_map_from_raw<PcmCode_UInt20_4, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt20_4_Le:
        return pcm_map_from_raw<PcmCode_UInt20_4, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt24:
        return pcm_map_from_raw<PcmCode_SInt24, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt24_Be:
        return pcm_map_from_raw<PcmCode_SInt24, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt24_Le:
        return pcm_map_from_raw<PcmCode_SInt24, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt24:
        return pcm_map_from_raw<PcmCode_UInt24, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt24_Be:
        return pcm_map_from_raw<PcmCode_UInt24, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt24_Le:
        return pcm_map_from_raw<PcmCode_UInt24, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt24_4:
        return pcm_map_from_raw<PcmCode_SInt24_4, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt24_4_Be:
        return pcm_map_from_raw<PcmCode_SInt24_4, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt24_4_Le:
        return pcm_map_from_raw<PcmCode_SInt24_4, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt24_4:
        return pcm_map_from_raw<PcmCode_UInt24_4, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt24_4_Be:
        return pcm_map_from_raw<PcmCode_UInt24_4, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt24_4_Le:
        return pcm_map_from_raw<PcmCode_UInt24_4, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt32:
        return pcm_map_from_raw<PcmCode_SInt32, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt32_Be:
        return pcm_map_from_raw<PcmCode_SInt32, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt32_Le:
        return pcm_map_from_raw<PcmCode_SInt32, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt32:
        return pcm_map_from_raw<PcmCode_UInt32, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt32_Be:
        return pcm_map_from_raw<PcmCode_UInt32, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt32_Le:
        return pcm_map_from_raw<PcmCode_UInt32, PcmEndian_Little>(in_format);
    case PcmSubformat_SInt64:
        return pcm_map_from_raw<PcmCode_SInt64, PcmEndian_Default>(in_format);
    case PcmSubformat_SInt64_Be:
        return pcm_map_from_raw<PcmCode_SInt64, PcmEndian_Big>(in_format);
    case PcmSubformat_SInt64_Le:
        return pcm_map_from_raw<PcmCode_SInt64, PcmEndian_Little>(in_format);
    case PcmSubformat_UInt64:
        return pcm_map_from_raw<PcmCode_UInt64, PcmEndian_Default>(in_format);
    case PcmSubformat_UInt64_Be:
        return pcm_map_from_raw<PcmCode_UInt64, PcmEndian_Big>(in_format);
    case PcmSubformat_UInt64_Le:
        return pcm_map_from_raw<PcmCode_UInt64, PcmEndian_Little>(in_format);
#if ROC_CPU_ENDIAN != ROC_CPU_BE
    case PcmSubformat_Float32_Be:
        return pcm_map_from_raw<PcmCode_Float32, PcmEndian_Big>(in_format);
#else
    case PcmSubformat_Float32_Le:
        return pcm_map_from_raw<PcmCode_Float32, PcmEndian_Little>(in_format);
#endif
    case PcmSubformat_Float64:
        return pcm_map_from_raw<PcmCode_Float64, PcmEndian_Default>(in_format);
    case PcmSubformat_Float64_Be:
        return pcm_map_from_raw<PcmCode_Float64, PcmEndian_Big>(in_format);
    case PcmSubformat_Float64_Le:
        return pcm_map_from_raw<PcmCode_Float64, PcmEndian_Little>(in_format);
    default:
        break;
    }

    // raw to raw
    switch (out_format) {
    case PcmSubformat_Float32:
        return pcm_map_from_raw<PcmCode_Float32, PcmEndian_Default>(in_format);
#if ROC_CPU_ENDIAN == ROC_CPU_BE
    case PcmSubformat_Float32_Be:
        return pcm_map_from_raw<PcmCode_Float32, PcmEndian_Default>(in_format);
#else
    case PcmSubformat_Float32_Le:
        return pcm_map_from_raw<PcmCode_Float32, PcmEndian_Default>(in_format);
#endif
    default:
        break;
    }

    return NULL;
}

// Get format traits
PcmTraits pcm_subformat_traits(PcmSubformat format) {
    PcmTraits traits;

    switch (format) {
    case PcmSubformat_SInt8:
        traits.id = PcmSubformat_SInt8;
        traits.name = "s8";
        traits.bit_width = 8;
        traits.bit_depth = 8;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt8_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt8_Le;
#endif
        traits.native_alias = PcmSubformat_SInt8;
        traits.default_variant = PcmSubformat_SInt8;
        traits.be_variant = PcmSubformat_SInt8_Be;
        traits.le_variant = PcmSubformat_SInt8_Le;
        break;

    case PcmSubformat_SInt8_Be:
        traits.id = PcmSubformat_SInt8_Be;
        traits.name = "s8_be";
        traits.bit_width = 8;
        traits.bit_depth = 8;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt8;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt8_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt8_Be;
        traits.default_variant = PcmSubformat_SInt8;
        traits.be_variant = PcmSubformat_SInt8_Be;
        traits.le_variant = PcmSubformat_SInt8_Le;
        break;

    case PcmSubformat_SInt8_Le:
        traits.id = PcmSubformat_SInt8_Le;
        traits.name = "s8_le";
        traits.bit_width = 8;
        traits.bit_depth = 8;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt8;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt8_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt8_Le;
        traits.default_variant = PcmSubformat_SInt8;
        traits.be_variant = PcmSubformat_SInt8_Be;
        traits.le_variant = PcmSubformat_SInt8_Le;
        break;

    case PcmSubformat_UInt8:
        traits.id = PcmSubformat_UInt8;
        traits.name = "u8";
        traits.bit_width = 8;
        traits.bit_depth = 8;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt8_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt8_Le;
#endif
        traits.native_alias = PcmSubformat_UInt8;
        traits.default_variant = PcmSubformat_UInt8;
        traits.be_variant = PcmSubformat_UInt8_Be;
        traits.le_variant = PcmSubformat_UInt8_Le;
        break;

    case PcmSubformat_UInt8_Be:
        traits.id = PcmSubformat_UInt8_Be;
        traits.name = "u8_be";
        traits.bit_width = 8;
        traits.bit_depth = 8;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt8;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt8_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt8_Be;
        traits.default_variant = PcmSubformat_UInt8;
        traits.be_variant = PcmSubformat_UInt8_Be;
        traits.le_variant = PcmSubformat_UInt8_Le;
        break;

    case PcmSubformat_UInt8_Le:
        traits.id = PcmSubformat_UInt8_Le;
        traits.name = "u8_le";
        traits.bit_width = 8;
        traits.bit_depth = 8;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt8;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt8_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt8_Le;
        traits.default_variant = PcmSubformat_UInt8;
        traits.be_variant = PcmSubformat_UInt8_Be;
        traits.le_variant = PcmSubformat_UInt8_Le;
        break;

    case PcmSubformat_SInt16:
        traits.id = PcmSubformat_SInt16;
        traits.name = "s16";
        traits.bit_width = 16;
        traits.bit_depth = 16;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt16_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt16_Le;
#endif
        traits.native_alias = PcmSubformat_SInt16;
        traits.default_variant = PcmSubformat_SInt16;
        traits.be_variant = PcmSubformat_SInt16_Be;
        traits.le_variant = PcmSubformat_SInt16_Le;
        break;

    case PcmSubformat_SInt16_Be:
        traits.id = PcmSubformat_SInt16_Be;
        traits.name = "s16_be";
        traits.bit_width = 16;
        traits.bit_depth = 16;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt16;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt16_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt16_Be;
        traits.default_variant = PcmSubformat_SInt16;
        traits.be_variant = PcmSubformat_SInt16_Be;
        traits.le_variant = PcmSubformat_SInt16_Le;
        break;

    case PcmSubformat_SInt16_Le:
        traits.id = PcmSubformat_SInt16_Le;
        traits.name = "s16_le";
        traits.bit_width = 16;
        traits.bit_depth = 16;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt16;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt16_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt16_Le;
        traits.default_variant = PcmSubformat_SInt16;
        traits.be_variant = PcmSubformat_SInt16_Be;
        traits.le_variant = PcmSubformat_SInt16_Le;
        break;

    case PcmSubformat_UInt16:
        traits.id = PcmSubformat_UInt16;
        traits.name = "u16";
        traits.bit_width = 16;
        traits.bit_depth = 16;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt16_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt16_Le;
#endif
        traits.native_alias = PcmSubformat_UInt16;
        traits.default_variant = PcmSubformat_UInt16;
        traits.be_variant = PcmSubformat_UInt16_Be;
        traits.le_variant = PcmSubformat_UInt16_Le;
        break;

    case PcmSubformat_UInt16_Be:
        traits.id = PcmSubformat_UInt16_Be;
        traits.name = "u16_be";
        traits.bit_width = 16;
        traits.bit_depth = 16;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt16;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt16_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt16_Be;
        traits.default_variant = PcmSubformat_UInt16;
        traits.be_variant = PcmSubformat_UInt16_Be;
        traits.le_variant = PcmSubformat_UInt16_Le;
        break;

    case PcmSubformat_UInt16_Le:
        traits.id = PcmSubformat_UInt16_Le;
        traits.name = "u16_le";
        traits.bit_width = 16;
        traits.bit_depth = 16;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt16;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt16_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt16_Le;
        traits.default_variant = PcmSubformat_UInt16;
        traits.be_variant = PcmSubformat_UInt16_Be;
        traits.le_variant = PcmSubformat_UInt16_Le;
        break;

    case PcmSubformat_SInt18:
        traits.id = PcmSubformat_SInt18;
        traits.name = "s18";
        traits.bit_width = 18;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt18_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt18_Le;
#endif
        traits.native_alias = PcmSubformat_SInt18;
        traits.default_variant = PcmSubformat_SInt18;
        traits.be_variant = PcmSubformat_SInt18_Be;
        traits.le_variant = PcmSubformat_SInt18_Le;
        break;

    case PcmSubformat_SInt18_Be:
        traits.id = PcmSubformat_SInt18_Be;
        traits.name = "s18_be";
        traits.bit_width = 18;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt18;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt18_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt18_Be;
        traits.default_variant = PcmSubformat_SInt18;
        traits.be_variant = PcmSubformat_SInt18_Be;
        traits.le_variant = PcmSubformat_SInt18_Le;
        break;

    case PcmSubformat_SInt18_Le:
        traits.id = PcmSubformat_SInt18_Le;
        traits.name = "s18_le";
        traits.bit_width = 18;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt18;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt18_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt18_Le;
        traits.default_variant = PcmSubformat_SInt18;
        traits.be_variant = PcmSubformat_SInt18_Be;
        traits.le_variant = PcmSubformat_SInt18_Le;
        break;

    case PcmSubformat_UInt18:
        traits.id = PcmSubformat_UInt18;
        traits.name = "u18";
        traits.bit_width = 18;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt18_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt18_Le;
#endif
        traits.native_alias = PcmSubformat_UInt18;
        traits.default_variant = PcmSubformat_UInt18;
        traits.be_variant = PcmSubformat_UInt18_Be;
        traits.le_variant = PcmSubformat_UInt18_Le;
        break;

    case PcmSubformat_UInt18_Be:
        traits.id = PcmSubformat_UInt18_Be;
        traits.name = "u18_be";
        traits.bit_width = 18;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt18;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt18_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt18_Be;
        traits.default_variant = PcmSubformat_UInt18;
        traits.be_variant = PcmSubformat_UInt18_Be;
        traits.le_variant = PcmSubformat_UInt18_Le;
        break;

    case PcmSubformat_UInt18_Le:
        traits.id = PcmSubformat_UInt18_Le;
        traits.name = "u18_le";
        traits.bit_width = 18;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt18;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt18_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt18_Le;
        traits.default_variant = PcmSubformat_UInt18;
        traits.be_variant = PcmSubformat_UInt18_Be;
        traits.le_variant = PcmSubformat_UInt18_Le;
        break;

    case PcmSubformat_SInt18_3:
        traits.id = PcmSubformat_SInt18_3;
        traits.name = "s18_3";
        traits.bit_width = 24;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt18_3_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt18_3_Le;
#endif
        traits.native_alias = PcmSubformat_SInt18_3;
        traits.default_variant = PcmSubformat_SInt18_3;
        traits.be_variant = PcmSubformat_SInt18_3_Be;
        traits.le_variant = PcmSubformat_SInt18_3_Le;
        break;

    case PcmSubformat_SInt18_3_Be:
        traits.id = PcmSubformat_SInt18_3_Be;
        traits.name = "s18_3be";
        traits.bit_width = 24;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt18_3;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt18_3_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt18_3_Be;
        traits.default_variant = PcmSubformat_SInt18_3;
        traits.be_variant = PcmSubformat_SInt18_3_Be;
        traits.le_variant = PcmSubformat_SInt18_3_Le;
        break;

    case PcmSubformat_SInt18_3_Le:
        traits.id = PcmSubformat_SInt18_3_Le;
        traits.name = "s18_3le";
        traits.bit_width = 24;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt18_3;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt18_3_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt18_3_Le;
        traits.default_variant = PcmSubformat_SInt18_3;
        traits.be_variant = PcmSubformat_SInt18_3_Be;
        traits.le_variant = PcmSubformat_SInt18_3_Le;
        break;

    case PcmSubformat_UInt18_3:
        traits.id = PcmSubformat_UInt18_3;
        traits.name = "u18_3";
        traits.bit_width = 24;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt18_3_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt18_3_Le;
#endif
        traits.native_alias = PcmSubformat_UInt18_3;
        traits.default_variant = PcmSubformat_UInt18_3;
        traits.be_variant = PcmSubformat_UInt18_3_Be;
        traits.le_variant = PcmSubformat_UInt18_3_Le;
        break;

    case PcmSubformat_UInt18_3_Be:
        traits.id = PcmSubformat_UInt18_3_Be;
        traits.name = "u18_3be";
        traits.bit_width = 24;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt18_3;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt18_3_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt18_3_Be;
        traits.default_variant = PcmSubformat_UInt18_3;
        traits.be_variant = PcmSubformat_UInt18_3_Be;
        traits.le_variant = PcmSubformat_UInt18_3_Le;
        break;

    case PcmSubformat_UInt18_3_Le:
        traits.id = PcmSubformat_UInt18_3_Le;
        traits.name = "u18_3le";
        traits.bit_width = 24;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt18_3;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt18_3_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt18_3_Le;
        traits.default_variant = PcmSubformat_UInt18_3;
        traits.be_variant = PcmSubformat_UInt18_3_Be;
        traits.le_variant = PcmSubformat_UInt18_3_Le;
        break;

    case PcmSubformat_SInt18_4:
        traits.id = PcmSubformat_SInt18_4;
        traits.name = "s18_4";
        traits.bit_width = 32;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt18_4_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt18_4_Le;
#endif
        traits.native_alias = PcmSubformat_SInt18_4;
        traits.default_variant = PcmSubformat_SInt18_4;
        traits.be_variant = PcmSubformat_SInt18_4_Be;
        traits.le_variant = PcmSubformat_SInt18_4_Le;
        break;

    case PcmSubformat_SInt18_4_Be:
        traits.id = PcmSubformat_SInt18_4_Be;
        traits.name = "s18_4be";
        traits.bit_width = 32;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt18_4;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt18_4_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt18_4_Be;
        traits.default_variant = PcmSubformat_SInt18_4;
        traits.be_variant = PcmSubformat_SInt18_4_Be;
        traits.le_variant = PcmSubformat_SInt18_4_Le;
        break;

    case PcmSubformat_SInt18_4_Le:
        traits.id = PcmSubformat_SInt18_4_Le;
        traits.name = "s18_4le";
        traits.bit_width = 32;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt18_4;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt18_4_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt18_4_Le;
        traits.default_variant = PcmSubformat_SInt18_4;
        traits.be_variant = PcmSubformat_SInt18_4_Be;
        traits.le_variant = PcmSubformat_SInt18_4_Le;
        break;

    case PcmSubformat_UInt18_4:
        traits.id = PcmSubformat_UInt18_4;
        traits.name = "u18_4";
        traits.bit_width = 32;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt18_4_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt18_4_Le;
#endif
        traits.native_alias = PcmSubformat_UInt18_4;
        traits.default_variant = PcmSubformat_UInt18_4;
        traits.be_variant = PcmSubformat_UInt18_4_Be;
        traits.le_variant = PcmSubformat_UInt18_4_Le;
        break;

    case PcmSubformat_UInt18_4_Be:
        traits.id = PcmSubformat_UInt18_4_Be;
        traits.name = "u18_4be";
        traits.bit_width = 32;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt18_4;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt18_4_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt18_4_Be;
        traits.default_variant = PcmSubformat_UInt18_4;
        traits.be_variant = PcmSubformat_UInt18_4_Be;
        traits.le_variant = PcmSubformat_UInt18_4_Le;
        break;

    case PcmSubformat_UInt18_4_Le:
        traits.id = PcmSubformat_UInt18_4_Le;
        traits.name = "u18_4le";
        traits.bit_width = 32;
        traits.bit_depth = 18;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt18_4;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt18_4_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt18_4_Le;
        traits.default_variant = PcmSubformat_UInt18_4;
        traits.be_variant = PcmSubformat_UInt18_4_Be;
        traits.le_variant = PcmSubformat_UInt18_4_Le;
        break;

    case PcmSubformat_SInt20:
        traits.id = PcmSubformat_SInt20;
        traits.name = "s20";
        traits.bit_width = 20;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt20_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt20_Le;
#endif
        traits.native_alias = PcmSubformat_SInt20;
        traits.default_variant = PcmSubformat_SInt20;
        traits.be_variant = PcmSubformat_SInt20_Be;
        traits.le_variant = PcmSubformat_SInt20_Le;
        break;

    case PcmSubformat_SInt20_Be:
        traits.id = PcmSubformat_SInt20_Be;
        traits.name = "s20_be";
        traits.bit_width = 20;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt20;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt20_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt20_Be;
        traits.default_variant = PcmSubformat_SInt20;
        traits.be_variant = PcmSubformat_SInt20_Be;
        traits.le_variant = PcmSubformat_SInt20_Le;
        break;

    case PcmSubformat_SInt20_Le:
        traits.id = PcmSubformat_SInt20_Le;
        traits.name = "s20_le";
        traits.bit_width = 20;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt20;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt20_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt20_Le;
        traits.default_variant = PcmSubformat_SInt20;
        traits.be_variant = PcmSubformat_SInt20_Be;
        traits.le_variant = PcmSubformat_SInt20_Le;
        break;

    case PcmSubformat_UInt20:
        traits.id = PcmSubformat_UInt20;
        traits.name = "u20";
        traits.bit_width = 20;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt20_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt20_Le;
#endif
        traits.native_alias = PcmSubformat_UInt20;
        traits.default_variant = PcmSubformat_UInt20;
        traits.be_variant = PcmSubformat_UInt20_Be;
        traits.le_variant = PcmSubformat_UInt20_Le;
        break;

    case PcmSubformat_UInt20_Be:
        traits.id = PcmSubformat_UInt20_Be;
        traits.name = "u20_be";
        traits.bit_width = 20;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt20;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt20_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt20_Be;
        traits.default_variant = PcmSubformat_UInt20;
        traits.be_variant = PcmSubformat_UInt20_Be;
        traits.le_variant = PcmSubformat_UInt20_Le;
        break;

    case PcmSubformat_UInt20_Le:
        traits.id = PcmSubformat_UInt20_Le;
        traits.name = "u20_le";
        traits.bit_width = 20;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt20;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt20_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt20_Le;
        traits.default_variant = PcmSubformat_UInt20;
        traits.be_variant = PcmSubformat_UInt20_Be;
        traits.le_variant = PcmSubformat_UInt20_Le;
        break;

    case PcmSubformat_SInt20_3:
        traits.id = PcmSubformat_SInt20_3;
        traits.name = "s20_3";
        traits.bit_width = 24;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt20_3_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt20_3_Le;
#endif
        traits.native_alias = PcmSubformat_SInt20_3;
        traits.default_variant = PcmSubformat_SInt20_3;
        traits.be_variant = PcmSubformat_SInt20_3_Be;
        traits.le_variant = PcmSubformat_SInt20_3_Le;
        break;

    case PcmSubformat_SInt20_3_Be:
        traits.id = PcmSubformat_SInt20_3_Be;
        traits.name = "s20_3be";
        traits.bit_width = 24;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt20_3;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt20_3_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt20_3_Be;
        traits.default_variant = PcmSubformat_SInt20_3;
        traits.be_variant = PcmSubformat_SInt20_3_Be;
        traits.le_variant = PcmSubformat_SInt20_3_Le;
        break;

    case PcmSubformat_SInt20_3_Le:
        traits.id = PcmSubformat_SInt20_3_Le;
        traits.name = "s20_3le";
        traits.bit_width = 24;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt20_3;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt20_3_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt20_3_Le;
        traits.default_variant = PcmSubformat_SInt20_3;
        traits.be_variant = PcmSubformat_SInt20_3_Be;
        traits.le_variant = PcmSubformat_SInt20_3_Le;
        break;

    case PcmSubformat_UInt20_3:
        traits.id = PcmSubformat_UInt20_3;
        traits.name = "u20_3";
        traits.bit_width = 24;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt20_3_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt20_3_Le;
#endif
        traits.native_alias = PcmSubformat_UInt20_3;
        traits.default_variant = PcmSubformat_UInt20_3;
        traits.be_variant = PcmSubformat_UInt20_3_Be;
        traits.le_variant = PcmSubformat_UInt20_3_Le;
        break;

    case PcmSubformat_UInt20_3_Be:
        traits.id = PcmSubformat_UInt20_3_Be;
        traits.name = "u20_3be";
        traits.bit_width = 24;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt20_3;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt20_3_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt20_3_Be;
        traits.default_variant = PcmSubformat_UInt20_3;
        traits.be_variant = PcmSubformat_UInt20_3_Be;
        traits.le_variant = PcmSubformat_UInt20_3_Le;
        break;

    case PcmSubformat_UInt20_3_Le:
        traits.id = PcmSubformat_UInt20_3_Le;
        traits.name = "u20_3le";
        traits.bit_width = 24;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt20_3;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt20_3_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt20_3_Le;
        traits.default_variant = PcmSubformat_UInt20_3;
        traits.be_variant = PcmSubformat_UInt20_3_Be;
        traits.le_variant = PcmSubformat_UInt20_3_Le;
        break;

    case PcmSubformat_SInt20_4:
        traits.id = PcmSubformat_SInt20_4;
        traits.name = "s20_4";
        traits.bit_width = 32;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt20_4_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt20_4_Le;
#endif
        traits.native_alias = PcmSubformat_SInt20_4;
        traits.default_variant = PcmSubformat_SInt20_4;
        traits.be_variant = PcmSubformat_SInt20_4_Be;
        traits.le_variant = PcmSubformat_SInt20_4_Le;
        break;

    case PcmSubformat_SInt20_4_Be:
        traits.id = PcmSubformat_SInt20_4_Be;
        traits.name = "s20_4be";
        traits.bit_width = 32;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt20_4;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt20_4_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt20_4_Be;
        traits.default_variant = PcmSubformat_SInt20_4;
        traits.be_variant = PcmSubformat_SInt20_4_Be;
        traits.le_variant = PcmSubformat_SInt20_4_Le;
        break;

    case PcmSubformat_SInt20_4_Le:
        traits.id = PcmSubformat_SInt20_4_Le;
        traits.name = "s20_4le";
        traits.bit_width = 32;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt20_4;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt20_4_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt20_4_Le;
        traits.default_variant = PcmSubformat_SInt20_4;
        traits.be_variant = PcmSubformat_SInt20_4_Be;
        traits.le_variant = PcmSubformat_SInt20_4_Le;
        break;

    case PcmSubformat_UInt20_4:
        traits.id = PcmSubformat_UInt20_4;
        traits.name = "u20_4";
        traits.bit_width = 32;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt20_4_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt20_4_Le;
#endif
        traits.native_alias = PcmSubformat_UInt20_4;
        traits.default_variant = PcmSubformat_UInt20_4;
        traits.be_variant = PcmSubformat_UInt20_4_Be;
        traits.le_variant = PcmSubformat_UInt20_4_Le;
        break;

    case PcmSubformat_UInt20_4_Be:
        traits.id = PcmSubformat_UInt20_4_Be;
        traits.name = "u20_4be";
        traits.bit_width = 32;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt20_4;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt20_4_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt20_4_Be;
        traits.default_variant = PcmSubformat_UInt20_4;
        traits.be_variant = PcmSubformat_UInt20_4_Be;
        traits.le_variant = PcmSubformat_UInt20_4_Le;
        break;

    case PcmSubformat_UInt20_4_Le:
        traits.id = PcmSubformat_UInt20_4_Le;
        traits.name = "u20_4le";
        traits.bit_width = 32;
        traits.bit_depth = 20;
        traits.flags = Pcm_IsInteger;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt20_4;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt20_4_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt20_4_Le;
        traits.default_variant = PcmSubformat_UInt20_4;
        traits.be_variant = PcmSubformat_UInt20_4_Be;
        traits.le_variant = PcmSubformat_UInt20_4_Le;
        break;

    case PcmSubformat_SInt24:
        traits.id = PcmSubformat_SInt24;
        traits.name = "s24";
        traits.bit_width = 24;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt24_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt24_Le;
#endif
        traits.native_alias = PcmSubformat_SInt24;
        traits.default_variant = PcmSubformat_SInt24;
        traits.be_variant = PcmSubformat_SInt24_Be;
        traits.le_variant = PcmSubformat_SInt24_Le;
        break;

    case PcmSubformat_SInt24_Be:
        traits.id = PcmSubformat_SInt24_Be;
        traits.name = "s24_be";
        traits.bit_width = 24;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt24;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt24_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt24_Be;
        traits.default_variant = PcmSubformat_SInt24;
        traits.be_variant = PcmSubformat_SInt24_Be;
        traits.le_variant = PcmSubformat_SInt24_Le;
        break;

    case PcmSubformat_SInt24_Le:
        traits.id = PcmSubformat_SInt24_Le;
        traits.name = "s24_le";
        traits.bit_width = 24;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt24;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt24_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt24_Le;
        traits.default_variant = PcmSubformat_SInt24;
        traits.be_variant = PcmSubformat_SInt24_Be;
        traits.le_variant = PcmSubformat_SInt24_Le;
        break;

    case PcmSubformat_UInt24:
        traits.id = PcmSubformat_UInt24;
        traits.name = "u24";
        traits.bit_width = 24;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt24_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt24_Le;
#endif
        traits.native_alias = PcmSubformat_UInt24;
        traits.default_variant = PcmSubformat_UInt24;
        traits.be_variant = PcmSubformat_UInt24_Be;
        traits.le_variant = PcmSubformat_UInt24_Le;
        break;

    case PcmSubformat_UInt24_Be:
        traits.id = PcmSubformat_UInt24_Be;
        traits.name = "u24_be";
        traits.bit_width = 24;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt24;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt24_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt24_Be;
        traits.default_variant = PcmSubformat_UInt24;
        traits.be_variant = PcmSubformat_UInt24_Be;
        traits.le_variant = PcmSubformat_UInt24_Le;
        break;

    case PcmSubformat_UInt24_Le:
        traits.id = PcmSubformat_UInt24_Le;
        traits.name = "u24_le";
        traits.bit_width = 24;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt24;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt24_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt24_Le;
        traits.default_variant = PcmSubformat_UInt24;
        traits.be_variant = PcmSubformat_UInt24_Be;
        traits.le_variant = PcmSubformat_UInt24_Le;
        break;

    case PcmSubformat_SInt24_4:
        traits.id = PcmSubformat_SInt24_4;
        traits.name = "s24_4";
        traits.bit_width = 32;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt24_4_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt24_4_Le;
#endif
        traits.native_alias = PcmSubformat_SInt24_4;
        traits.default_variant = PcmSubformat_SInt24_4;
        traits.be_variant = PcmSubformat_SInt24_4_Be;
        traits.le_variant = PcmSubformat_SInt24_4_Le;
        break;

    case PcmSubformat_SInt24_4_Be:
        traits.id = PcmSubformat_SInt24_4_Be;
        traits.name = "s24_4be";
        traits.bit_width = 32;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt24_4;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt24_4_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt24_4_Be;
        traits.default_variant = PcmSubformat_SInt24_4;
        traits.be_variant = PcmSubformat_SInt24_4_Be;
        traits.le_variant = PcmSubformat_SInt24_4_Le;
        break;

    case PcmSubformat_SInt24_4_Le:
        traits.id = PcmSubformat_SInt24_4_Le;
        traits.name = "s24_4le";
        traits.bit_width = 32;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt24_4;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt24_4_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt24_4_Le;
        traits.default_variant = PcmSubformat_SInt24_4;
        traits.be_variant = PcmSubformat_SInt24_4_Be;
        traits.le_variant = PcmSubformat_SInt24_4_Le;
        break;

    case PcmSubformat_UInt24_4:
        traits.id = PcmSubformat_UInt24_4;
        traits.name = "u24_4";
        traits.bit_width = 32;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt24_4_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt24_4_Le;
#endif
        traits.native_alias = PcmSubformat_UInt24_4;
        traits.default_variant = PcmSubformat_UInt24_4;
        traits.be_variant = PcmSubformat_UInt24_4_Be;
        traits.le_variant = PcmSubformat_UInt24_4_Le;
        break;

    case PcmSubformat_UInt24_4_Be:
        traits.id = PcmSubformat_UInt24_4_Be;
        traits.name = "u24_4be";
        traits.bit_width = 32;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt24_4;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt24_4_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt24_4_Be;
        traits.default_variant = PcmSubformat_UInt24_4;
        traits.be_variant = PcmSubformat_UInt24_4_Be;
        traits.le_variant = PcmSubformat_UInt24_4_Le;
        break;

    case PcmSubformat_UInt24_4_Le:
        traits.id = PcmSubformat_UInt24_4_Le;
        traits.name = "u24_4le";
        traits.bit_width = 32;
        traits.bit_depth = 24;
        traits.flags = Pcm_IsInteger | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt24_4;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt24_4_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt24_4_Le;
        traits.default_variant = PcmSubformat_UInt24_4;
        traits.be_variant = PcmSubformat_UInt24_4_Be;
        traits.le_variant = PcmSubformat_UInt24_4_Le;
        break;

    case PcmSubformat_SInt32:
        traits.id = PcmSubformat_SInt32;
        traits.name = "s32";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt32_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt32_Le;
#endif
        traits.native_alias = PcmSubformat_SInt32;
        traits.default_variant = PcmSubformat_SInt32;
        traits.be_variant = PcmSubformat_SInt32_Be;
        traits.le_variant = PcmSubformat_SInt32_Le;
        break;

    case PcmSubformat_SInt32_Be:
        traits.id = PcmSubformat_SInt32_Be;
        traits.name = "s32_be";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt32;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt32_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt32_Be;
        traits.default_variant = PcmSubformat_SInt32;
        traits.be_variant = PcmSubformat_SInt32_Be;
        traits.le_variant = PcmSubformat_SInt32_Le;
        break;

    case PcmSubformat_SInt32_Le:
        traits.id = PcmSubformat_SInt32_Le;
        traits.name = "s32_le";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt32;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt32_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt32_Le;
        traits.default_variant = PcmSubformat_SInt32;
        traits.be_variant = PcmSubformat_SInt32_Be;
        traits.le_variant = PcmSubformat_SInt32_Le;
        break;

    case PcmSubformat_UInt32:
        traits.id = PcmSubformat_UInt32;
        traits.name = "u32";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt32_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt32_Le;
#endif
        traits.native_alias = PcmSubformat_UInt32;
        traits.default_variant = PcmSubformat_UInt32;
        traits.be_variant = PcmSubformat_UInt32_Be;
        traits.le_variant = PcmSubformat_UInt32_Le;
        break;

    case PcmSubformat_UInt32_Be:
        traits.id = PcmSubformat_UInt32_Be;
        traits.name = "u32_be";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt32;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt32_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt32_Be;
        traits.default_variant = PcmSubformat_UInt32;
        traits.be_variant = PcmSubformat_UInt32_Be;
        traits.le_variant = PcmSubformat_UInt32_Le;
        break;

    case PcmSubformat_UInt32_Le:
        traits.id = PcmSubformat_UInt32_Le;
        traits.name = "u32_le";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt32;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt32_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt32_Le;
        traits.default_variant = PcmSubformat_UInt32;
        traits.be_variant = PcmSubformat_UInt32_Be;
        traits.le_variant = PcmSubformat_UInt32_Le;
        break;

    case PcmSubformat_SInt64:
        traits.id = PcmSubformat_SInt64;
        traits.name = "s64";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_SInt64_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_SInt64_Le;
#endif
        traits.native_alias = PcmSubformat_SInt64;
        traits.default_variant = PcmSubformat_SInt64;
        traits.be_variant = PcmSubformat_SInt64_Be;
        traits.le_variant = PcmSubformat_SInt64_Le;
        break;

    case PcmSubformat_SInt64_Be:
        traits.id = PcmSubformat_SInt64_Be;
        traits.name = "s64_be";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt64;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_SInt64_Be;
#endif
        traits.portable_alias = PcmSubformat_SInt64_Be;
        traits.default_variant = PcmSubformat_SInt64;
        traits.be_variant = PcmSubformat_SInt64_Be;
        traits.le_variant = PcmSubformat_SInt64_Le;
        break;

    case PcmSubformat_SInt64_Le:
        traits.id = PcmSubformat_SInt64_Le;
        traits.name = "s64_le";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsInteger | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt64;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_SInt64_Le;
#endif
        traits.portable_alias = PcmSubformat_SInt64_Le;
        traits.default_variant = PcmSubformat_SInt64;
        traits.be_variant = PcmSubformat_SInt64_Be;
        traits.le_variant = PcmSubformat_SInt64_Le;
        break;

    case PcmSubformat_UInt64:
        traits.id = PcmSubformat_UInt64;
        traits.name = "u64";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_UInt64_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_UInt64_Le;
#endif
        traits.native_alias = PcmSubformat_UInt64;
        traits.default_variant = PcmSubformat_UInt64;
        traits.be_variant = PcmSubformat_UInt64_Be;
        traits.le_variant = PcmSubformat_UInt64_Le;
        break;

    case PcmSubformat_UInt64_Be:
        traits.id = PcmSubformat_UInt64_Be;
        traits.name = "u64_be";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt64;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_UInt64_Be;
#endif
        traits.portable_alias = PcmSubformat_UInt64_Be;
        traits.default_variant = PcmSubformat_UInt64;
        traits.be_variant = PcmSubformat_UInt64_Be;
        traits.le_variant = PcmSubformat_UInt64_Le;
        break;

    case PcmSubformat_UInt64_Le:
        traits.id = PcmSubformat_UInt64_Le;
        traits.name = "u64_le";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsInteger | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt64;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_UInt64_Le;
#endif
        traits.portable_alias = PcmSubformat_UInt64_Le;
        traits.default_variant = PcmSubformat_UInt64;
        traits.be_variant = PcmSubformat_UInt64_Be;
        traits.le_variant = PcmSubformat_UInt64_Le;
        break;

    case PcmSubformat_Float32:
        traits.id = PcmSubformat_Float32;
        traits.name = "f32";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsFloat | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_Float32_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_Float32_Le;
#endif
        traits.native_alias = PcmSubformat_Float32;
        traits.default_variant = PcmSubformat_Float32;
        traits.be_variant = PcmSubformat_Float32_Be;
        traits.le_variant = PcmSubformat_Float32_Le;
        break;

    case PcmSubformat_Float32_Be:
        traits.id = PcmSubformat_Float32_Be;
        traits.name = "f32_be";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsFloat | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_Float32;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_Float32_Be;
#endif
        traits.portable_alias = PcmSubformat_Float32_Be;
        traits.default_variant = PcmSubformat_Float32;
        traits.be_variant = PcmSubformat_Float32_Be;
        traits.le_variant = PcmSubformat_Float32_Le;
        break;

    case PcmSubformat_Float32_Le:
        traits.id = PcmSubformat_Float32_Le;
        traits.name = "f32_le";
        traits.bit_width = 32;
        traits.bit_depth = 32;
        traits.flags = Pcm_IsFloat | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_Float32;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_Float32_Le;
#endif
        traits.portable_alias = PcmSubformat_Float32_Le;
        traits.default_variant = PcmSubformat_Float32;
        traits.be_variant = PcmSubformat_Float32_Be;
        traits.le_variant = PcmSubformat_Float32_Le;
        break;

    case PcmSubformat_Float64:
        traits.id = PcmSubformat_Float64;
        traits.name = "f64";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsFloat | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.portable_alias = PcmSubformat_Float64_Be;
#else
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.portable_alias = PcmSubformat_Float64_Le;
#endif
        traits.native_alias = PcmSubformat_Float64;
        traits.default_variant = PcmSubformat_Float64;
        traits.be_variant = PcmSubformat_Float64_Be;
        traits.le_variant = PcmSubformat_Float64_Le;
        break;

    case PcmSubformat_Float64_Be:
        traits.id = PcmSubformat_Float64_Be;
        traits.name = "f64_be";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsFloat | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_BE
        traits.flags |= Pcm_IsNative | Pcm_IsBig;
        traits.native_alias = PcmSubformat_Float64;
#else
        traits.flags |= Pcm_IsBig;
        traits.native_alias = PcmSubformat_Float64_Be;
#endif
        traits.portable_alias = PcmSubformat_Float64_Be;
        traits.default_variant = PcmSubformat_Float64;
        traits.be_variant = PcmSubformat_Float64_Be;
        traits.le_variant = PcmSubformat_Float64_Le;
        break;

    case PcmSubformat_Float64_Le:
        traits.id = PcmSubformat_Float64_Le;
        traits.name = "f64_le";
        traits.bit_width = 64;
        traits.bit_depth = 64;
        traits.flags = Pcm_IsFloat | Pcm_IsSigned | Pcm_IsPacked | Pcm_IsAligned;
#if ROC_CPU_ENDIAN == ROC_CPU_LE
        traits.flags |= Pcm_IsNative | Pcm_IsLittle;
        traits.native_alias = PcmSubformat_Float64;
#else
        traits.flags |= Pcm_IsLittle;
        traits.native_alias = PcmSubformat_Float64_Le;
#endif
        traits.portable_alias = PcmSubformat_Float64_Le;
        traits.default_variant = PcmSubformat_Float64;
        traits.be_variant = PcmSubformat_Float64_Be;
        traits.le_variant = PcmSubformat_Float64_Le;
        break;

    default:
        break;
    }

    return traits;
}

const char* pcm_subformat_to_str(PcmSubformat format) {
    switch (format) {
    case PcmSubformat_SInt8:
        return "s8";
    case PcmSubformat_SInt8_Be:
        return "s8_be";
    case PcmSubformat_SInt8_Le:
        return "s8_le";
    case PcmSubformat_UInt8:
        return "u8";
    case PcmSubformat_UInt8_Be:
        return "u8_be";
    case PcmSubformat_UInt8_Le:
        return "u8_le";
    case PcmSubformat_SInt16:
        return "s16";
    case PcmSubformat_SInt16_Be:
        return "s16_be";
    case PcmSubformat_SInt16_Le:
        return "s16_le";
    case PcmSubformat_UInt16:
        return "u16";
    case PcmSubformat_UInt16_Be:
        return "u16_be";
    case PcmSubformat_UInt16_Le:
        return "u16_le";
    case PcmSubformat_SInt18:
        return "s18";
    case PcmSubformat_SInt18_Be:
        return "s18_be";
    case PcmSubformat_SInt18_Le:
        return "s18_le";
    case PcmSubformat_UInt18:
        return "u18";
    case PcmSubformat_UInt18_Be:
        return "u18_be";
    case PcmSubformat_UInt18_Le:
        return "u18_le";
    case PcmSubformat_SInt18_3:
        return "s18_3";
    case PcmSubformat_SInt18_3_Be:
        return "s18_3be";
    case PcmSubformat_SInt18_3_Le:
        return "s18_3le";
    case PcmSubformat_UInt18_3:
        return "u18_3";
    case PcmSubformat_UInt18_3_Be:
        return "u18_3be";
    case PcmSubformat_UInt18_3_Le:
        return "u18_3le";
    case PcmSubformat_SInt18_4:
        return "s18_4";
    case PcmSubformat_SInt18_4_Be:
        return "s18_4be";
    case PcmSubformat_SInt18_4_Le:
        return "s18_4le";
    case PcmSubformat_UInt18_4:
        return "u18_4";
    case PcmSubformat_UInt18_4_Be:
        return "u18_4be";
    case PcmSubformat_UInt18_4_Le:
        return "u18_4le";
    case PcmSubformat_SInt20:
        return "s20";
    case PcmSubformat_SInt20_Be:
        return "s20_be";
    case PcmSubformat_SInt20_Le:
        return "s20_le";
    case PcmSubformat_UInt20:
        return "u20";
    case PcmSubformat_UInt20_Be:
        return "u20_be";
    case PcmSubformat_UInt20_Le:
        return "u20_le";
    case PcmSubformat_SInt20_3:
        return "s20_3";
    case PcmSubformat_SInt20_3_Be:
        return "s20_3be";
    case PcmSubformat_SInt20_3_Le:
        return "s20_3le";
    case PcmSubformat_UInt20_3:
        return "u20_3";
    case PcmSubformat_UInt20_3_Be:
        return "u20_3be";
    case PcmSubformat_UInt20_3_Le:
        return "u20_3le";
    case PcmSubformat_SInt20_4:
        return "s20_4";
    case PcmSubformat_SInt20_4_Be:
        return "s20_4be";
    case PcmSubformat_SInt20_4_Le:
        return "s20_4le";
    case PcmSubformat_UInt20_4:
        return "u20_4";
    case PcmSubformat_UInt20_4_Be:
        return "u20_4be";
    case PcmSubformat_UInt20_4_Le:
        return "u20_4le";
    case PcmSubformat_SInt24:
        return "s24";
    case PcmSubformat_SInt24_Be:
        return "s24_be";
    case PcmSubformat_SInt24_Le:
        return "s24_le";
    case PcmSubformat_UInt24:
        return "u24";
    case PcmSubformat_UInt24_Be:
        return "u24_be";
    case PcmSubformat_UInt24_Le:
        return "u24_le";
    case PcmSubformat_SInt24_4:
        return "s24_4";
    case PcmSubformat_SInt24_4_Be:
        return "s24_4be";
    case PcmSubformat_SInt24_4_Le:
        return "s24_4le";
    case PcmSubformat_UInt24_4:
        return "u24_4";
    case PcmSubformat_UInt24_4_Be:
        return "u24_4be";
    case PcmSubformat_UInt24_4_Le:
        return "u24_4le";
    case PcmSubformat_SInt32:
        return "s32";
    case PcmSubformat_SInt32_Be:
        return "s32_be";
    case PcmSubformat_SInt32_Le:
        return "s32_le";
    case PcmSubformat_UInt32:
        return "u32";
    case PcmSubformat_UInt32_Be:
        return "u32_be";
    case PcmSubformat_UInt32_Le:
        return "u32_le";
    case PcmSubformat_SInt64:
        return "s64";
    case PcmSubformat_SInt64_Be:
        return "s64_be";
    case PcmSubformat_SInt64_Le:
        return "s64_le";
    case PcmSubformat_UInt64:
        return "u64";
    case PcmSubformat_UInt64_Be:
        return "u64_be";
    case PcmSubformat_UInt64_Le:
        return "u64_le";
    case PcmSubformat_Float32:
        return "f32";
    case PcmSubformat_Float32_Be:
        return "f32_be";
    case PcmSubformat_Float32_Le:
        return "f32_le";
    case PcmSubformat_Float64:
        return "f64";
    case PcmSubformat_Float64_Be:
        return "f64_be";
    case PcmSubformat_Float64_Le:
        return "f64_le";
    default:
        break;
    }
    return NULL;
}

PcmSubformat pcm_subformat_from_str(const char* str) {
    if (!str) {
        return PcmSubformat_Invalid;
    }
    if (str[0] == 'f') {
        if (str[1] == '3') {
            if (str[2] == '2') {
                if (strcmp(str, "f32") == 0) {
                    return PcmSubformat_Float32;
                }
                if (strcmp(str, "f32_be") == 0) {
                    return PcmSubformat_Float32_Be;
                }
                if (strcmp(str, "f32_le") == 0) {
                    return PcmSubformat_Float32_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '6') {
            if (str[2] == '4') {
                if (strcmp(str, "f64") == 0) {
                    return PcmSubformat_Float64;
                }
                if (strcmp(str, "f64_be") == 0) {
                    return PcmSubformat_Float64_Be;
                }
                if (strcmp(str, "f64_le") == 0) {
                    return PcmSubformat_Float64_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        return PcmSubformat_Invalid;
    }
    if (str[0] == 's') {
        if (str[1] == '1') {
            if (str[2] == '6') {
                if (strcmp(str, "s16") == 0) {
                    return PcmSubformat_SInt16;
                }
                if (strcmp(str, "s16_be") == 0) {
                    return PcmSubformat_SInt16_Be;
                }
                if (strcmp(str, "s16_le") == 0) {
                    return PcmSubformat_SInt16_Le;
                }
                return PcmSubformat_Invalid;
            }
            if (str[2] == '8') {
                if (strcmp(str, "s18") == 0) {
                    return PcmSubformat_SInt18;
                }
                if (strcmp(str, "s18_be") == 0) {
                    return PcmSubformat_SInt18_Be;
                }
                if (strcmp(str, "s18_le") == 0) {
                    return PcmSubformat_SInt18_Le;
                }
                if (strcmp(str, "s18_3") == 0) {
                    return PcmSubformat_SInt18_3;
                }
                if (strcmp(str, "s18_3be") == 0) {
                    return PcmSubformat_SInt18_3_Be;
                }
                if (strcmp(str, "s18_3le") == 0) {
                    return PcmSubformat_SInt18_3_Le;
                }
                if (strcmp(str, "s18_4") == 0) {
                    return PcmSubformat_SInt18_4;
                }
                if (strcmp(str, "s18_4be") == 0) {
                    return PcmSubformat_SInt18_4_Be;
                }
                if (strcmp(str, "s18_4le") == 0) {
                    return PcmSubformat_SInt18_4_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '2') {
            if (str[2] == '0') {
                if (strcmp(str, "s20") == 0) {
                    return PcmSubformat_SInt20;
                }
                if (strcmp(str, "s20_be") == 0) {
                    return PcmSubformat_SInt20_Be;
                }
                if (strcmp(str, "s20_le") == 0) {
                    return PcmSubformat_SInt20_Le;
                }
                if (strcmp(str, "s20_3") == 0) {
                    return PcmSubformat_SInt20_3;
                }
                if (strcmp(str, "s20_3be") == 0) {
                    return PcmSubformat_SInt20_3_Be;
                }
                if (strcmp(str, "s20_3le") == 0) {
                    return PcmSubformat_SInt20_3_Le;
                }
                if (strcmp(str, "s20_4") == 0) {
                    return PcmSubformat_SInt20_4;
                }
                if (strcmp(str, "s20_4be") == 0) {
                    return PcmSubformat_SInt20_4_Be;
                }
                if (strcmp(str, "s20_4le") == 0) {
                    return PcmSubformat_SInt20_4_Le;
                }
                return PcmSubformat_Invalid;
            }
            if (str[2] == '4') {
                if (strcmp(str, "s24") == 0) {
                    return PcmSubformat_SInt24;
                }
                if (strcmp(str, "s24_be") == 0) {
                    return PcmSubformat_SInt24_Be;
                }
                if (strcmp(str, "s24_le") == 0) {
                    return PcmSubformat_SInt24_Le;
                }
                if (strcmp(str, "s24_4") == 0) {
                    return PcmSubformat_SInt24_4;
                }
                if (strcmp(str, "s24_4be") == 0) {
                    return PcmSubformat_SInt24_4_Be;
                }
                if (strcmp(str, "s24_4le") == 0) {
                    return PcmSubformat_SInt24_4_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '3') {
            if (str[2] == '2') {
                if (strcmp(str, "s32") == 0) {
                    return PcmSubformat_SInt32;
                }
                if (strcmp(str, "s32_be") == 0) {
                    return PcmSubformat_SInt32_Be;
                }
                if (strcmp(str, "s32_le") == 0) {
                    return PcmSubformat_SInt32_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '6') {
            if (str[2] == '4') {
                if (strcmp(str, "s64") == 0) {
                    return PcmSubformat_SInt64;
                }
                if (strcmp(str, "s64_be") == 0) {
                    return PcmSubformat_SInt64_Be;
                }
                if (strcmp(str, "s64_le") == 0) {
                    return PcmSubformat_SInt64_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '8') {
            if (strcmp(str, "s8") == 0) {
                return PcmSubformat_SInt8;
            }
            if (strcmp(str, "s8_be") == 0) {
                return PcmSubformat_SInt8_Be;
            }
            if (strcmp(str, "s8_le") == 0) {
                return PcmSubformat_SInt8_Le;
            }
            return PcmSubformat_Invalid;
        }
        return PcmSubformat_Invalid;
    }
    if (str[0] == 'u') {
        if (str[1] == '1') {
            if (str[2] == '6') {
                if (strcmp(str, "u16") == 0) {
                    return PcmSubformat_UInt16;
                }
                if (strcmp(str, "u16_be") == 0) {
                    return PcmSubformat_UInt16_Be;
                }
                if (strcmp(str, "u16_le") == 0) {
                    return PcmSubformat_UInt16_Le;
                }
                return PcmSubformat_Invalid;
            }
            if (str[2] == '8') {
                if (strcmp(str, "u18") == 0) {
                    return PcmSubformat_UInt18;
                }
                if (strcmp(str, "u18_be") == 0) {
                    return PcmSubformat_UInt18_Be;
                }
                if (strcmp(str, "u18_le") == 0) {
                    return PcmSubformat_UInt18_Le;
                }
                if (strcmp(str, "u18_3") == 0) {
                    return PcmSubformat_UInt18_3;
                }
                if (strcmp(str, "u18_3be") == 0) {
                    return PcmSubformat_UInt18_3_Be;
                }
                if (strcmp(str, "u18_3le") == 0) {
                    return PcmSubformat_UInt18_3_Le;
                }
                if (strcmp(str, "u18_4") == 0) {
                    return PcmSubformat_UInt18_4;
                }
                if (strcmp(str, "u18_4be") == 0) {
                    return PcmSubformat_UInt18_4_Be;
                }
                if (strcmp(str, "u18_4le") == 0) {
                    return PcmSubformat_UInt18_4_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '2') {
            if (str[2] == '0') {
                if (strcmp(str, "u20") == 0) {
                    return PcmSubformat_UInt20;
                }
                if (strcmp(str, "u20_be") == 0) {
                    return PcmSubformat_UInt20_Be;
                }
                if (strcmp(str, "u20_le") == 0) {
                    return PcmSubformat_UInt20_Le;
                }
                if (strcmp(str, "u20_3") == 0) {
                    return PcmSubformat_UInt20_3;
                }
                if (strcmp(str, "u20_3be") == 0) {
                    return PcmSubformat_UInt20_3_Be;
                }
                if (strcmp(str, "u20_3le") == 0) {
                    return PcmSubformat_UInt20_3_Le;
                }
                if (strcmp(str, "u20_4") == 0) {
                    return PcmSubformat_UInt20_4;
                }
                if (strcmp(str, "u20_4be") == 0) {
                    return PcmSubformat_UInt20_4_Be;
                }
                if (strcmp(str, "u20_4le") == 0) {
                    return PcmSubformat_UInt20_4_Le;
                }
                return PcmSubformat_Invalid;
            }
            if (str[2] == '4') {
                if (strcmp(str, "u24") == 0) {
                    return PcmSubformat_UInt24;
                }
                if (strcmp(str, "u24_be") == 0) {
                    return PcmSubformat_UInt24_Be;
                }
                if (strcmp(str, "u24_le") == 0) {
                    return PcmSubformat_UInt24_Le;
                }
                if (strcmp(str, "u24_4") == 0) {
                    return PcmSubformat_UInt24_4;
                }
                if (strcmp(str, "u24_4be") == 0) {
                    return PcmSubformat_UInt24_4_Be;
                }
                if (strcmp(str, "u24_4le") == 0) {
                    return PcmSubformat_UInt24_4_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '3') {
            if (str[2] == '2') {
                if (strcmp(str, "u32") == 0) {
                    return PcmSubformat_UInt32;
                }
                if (strcmp(str, "u32_be") == 0) {
                    return PcmSubformat_UInt32_Be;
                }
                if (strcmp(str, "u32_le") == 0) {
                    return PcmSubformat_UInt32_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '6') {
            if (str[2] == '4') {
                if (strcmp(str, "u64") == 0) {
                    return PcmSubformat_UInt64;
                }
                if (strcmp(str, "u64_be") == 0) {
                    return PcmSubformat_UInt64_Be;
                }
                if (strcmp(str, "u64_le") == 0) {
                    return PcmSubformat_UInt64_Le;
                }
                return PcmSubformat_Invalid;
            }
            return PcmSubformat_Invalid;
        }
        if (str[1] == '8') {
            if (strcmp(str, "u8") == 0) {
                return PcmSubformat_UInt8;
            }
            if (strcmp(str, "u8_be") == 0) {
                return PcmSubformat_UInt8_Be;
            }
            if (strcmp(str, "u8_le") == 0) {
                return PcmSubformat_UInt8_Le;
            }
            return PcmSubformat_Invalid;
        }
        return PcmSubformat_Invalid;
    }
    return PcmSubformat_Invalid;
}

} // namespace audio
} // namespace roc
