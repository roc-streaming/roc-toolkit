/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/cpu_defs.h
//! @brief CPU definitions.

#ifndef ROC_CORE_CPU_DEFS_H_
#define ROC_CORE_CPU_DEFS_H_

#ifdef ROC_DOXYGEN
//! Defines target CPU architecture.
//! Set to one of the ROC_CPU_FAMILY_xxx values at compile time.
//! If architecture can't be determined, it's set to ROC_CPU_FAMILY_GENERIC.
//!
//! We can have special handling for some architectures in code, but most often,
//! we only use it as a hint to determine CPU endian and bitness.
//!
//! Presence in the list doesn't mean that the architecture has actually been
//! tested, nor does absence mean that it's not supported.
#define ROC_CPU_FAMILY
#endif

#define ROC_CPU_FAMILY_GENERIC 0     //!< Unknown architecture.
#define ROC_CPU_FAMILY_X86_64 1      //!< x86-64 (AMD64/Intel 64) architecture.
#define ROC_CPU_FAMILY_X86 2         //!< x86 (IA-32) 32-bit architecture.
#define ROC_CPU_FAMILY_PPC64 3       //!< PowerPC 64-bit architecture.
#define ROC_CPU_FAMILY_PPC 4         //!< PowerPC 32-bit architecture.
#define ROC_CPU_FAMILY_S390X 5       //!< IBM System/390 64-bit architecture.
#define ROC_CPU_FAMILY_S390 6        //!< IBM System/390 32-bit architecture.
#define ROC_CPU_FAMILY_LOONGARCH64 7 //!< LoongArch 64-bit architecture.
#define ROC_CPU_FAMILY_LOONGARCH32 8 //!< LoongArch 32-bit architecture.
#define ROC_CPU_FAMILY_AARCH64 9     //!< ARM 64-bit architecture (ARMv8-A and above).
#define ROC_CPU_FAMILY_ARM 10        //!< ARM 32-bit architecture.
#define ROC_CPU_FAMILY_MIPS64 11     //!< MIPS 64-bit architecture.
#define ROC_CPU_FAMILY_MIPS 12       //!< MIPS 32-bit architecture.
#define ROC_CPU_FAMILY_RISCV64 13    //!< RISC-V 64-bit architecture.
#define ROC_CPU_FAMILY_RISCV32 14    //!< RISC-V 32-bit architecture.
#define ROC_CPU_FAMILY_MICROBLAZE 15 //!< Xilinx MicroBlaze soft processor core.
#define ROC_CPU_FAMILY_ARC 16        //!< Snopsys ARC processor.
#define ROC_CPU_FAMILY_CSKY 17       //!< C-SKY processor architecture.

#ifdef ROC_DOXYGEN
//! Defines target CPU endianness.
//! Set to one of the ROC_CPU_ENDIAN_xx values at compile time.
//! If endian can't be determined, compilation fails.
#define ROC_CPU_ENDIAN
#endif

#define ROC_CPU_ENDIAN_BE 1 //!< Big-endian CPU.
#define ROC_CPU_ENDIAN_LE 2 //!< Little-endian CPU.

#ifdef ROC_DOXYGEN
//! Defines target CPU bitness.
//! Set to 64 or 32 at compile time.
#define ROC_CPU_BITS
#endif

#endif // ROC_CORE_CPU_DEFS_H_
