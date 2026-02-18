// THIS FILE IS AUTO-GENERATED. DO NOT EDIT!
// See atomic_ops_gen.py

#ifndef ROC_CORE_ATOMIC_OPS_H_
#define ROC_CORE_ATOMIC_OPS_H_

#include "roc_core/stddefs.h"

#include <atomic_ops.h>

namespace roc {
namespace core {

//! Atomic operations.
class AtomicOps {
public:
    #if defined(AO_HAVE_nop_read)
    static inline void fence_acquire() {
        AO_nop_read();
    }
    #endif

    #if defined(AO_HAVE_nop_write)
    static inline void fence_release() {
        AO_nop_write();
    }
    #endif

    #if defined(AO_HAVE_nop_full)
    static inline void fence_seq_cst() {
        AO_nop_full();
    }
    #endif

    #if defined(AO_HAVE_char_load)
    template <class T = unsigned char>
    static inline unsigned char load_relaxed(unsigned char const& var) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_load((unsigned char const*)&var);
    }
    #endif

    #if defined(AO_HAVE_char_load)
    template <class T = signed char>
    static inline signed char load_relaxed(signed char const& var) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_load((unsigned char const*)&var);
    }
    #endif

    #if defined(AO_HAVE_short_load)
    template <class T = unsigned short>
    static inline unsigned short load_relaxed(unsigned short const& var) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_load((unsigned short const*)&var);
    }
    #endif

    #if defined(AO_HAVE_short_load)
    template <class T = short>
    static inline short load_relaxed(short const& var) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_load((unsigned short const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int load_relaxed(unsigned int const& var) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_load((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int load_relaxed(int const& var) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_load((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_int_load) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int load_relaxed(unsigned int const& var) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_load((unsigned int const*)&var);
    }
    #endif

    #if defined(AO_HAVE_int_load) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int load_relaxed(int const& var) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_load((unsigned int const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load)
    template <class T = size_t>
    static inline size_t load_relaxed(size_t const& var) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_load((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load)
    template <class T = ssize_t>
    static inline ssize_t load_relaxed(ssize_t const& var) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_load((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_char_load_acquire)
    template <class T = unsigned char>
    static inline unsigned char load_acquire(unsigned char const& var) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_load_acquire((unsigned char const*)&var);
    }
    #endif

    #if defined(AO_HAVE_char_load_acquire)
    template <class T = signed char>
    static inline signed char load_acquire(signed char const& var) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_load_acquire((unsigned char const*)&var);
    }
    #endif

    #if defined(AO_HAVE_short_load_acquire)
    template <class T = unsigned short>
    static inline unsigned short load_acquire(unsigned short const& var) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_load_acquire((unsigned short const*)&var);
    }
    #endif

    #if defined(AO_HAVE_short_load_acquire)
    template <class T = short>
    static inline short load_acquire(short const& var) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_load_acquire((unsigned short const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int load_acquire(unsigned int const& var) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_load_acquire((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int load_acquire(int const& var) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_load_acquire((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_int_load_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int load_acquire(unsigned int const& var) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_load_acquire((unsigned int const*)&var);
    }
    #endif

    #if defined(AO_HAVE_int_load_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int load_acquire(int const& var) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_load_acquire((unsigned int const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_acquire)
    template <class T = size_t>
    static inline size_t load_acquire(size_t const& var) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_load_acquire((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_acquire)
    template <class T = ssize_t>
    static inline ssize_t load_acquire(ssize_t const& var) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_load_acquire((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_char_load_full)
    template <class T = unsigned char>
    static inline unsigned char load_seq_cst(unsigned char const& var) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_load_full((unsigned char const*)&var);
    }
    #endif

    #if defined(AO_HAVE_char_load_full)
    template <class T = signed char>
    static inline signed char load_seq_cst(signed char const& var) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_load_full((unsigned char const*)&var);
    }
    #endif

    #if defined(AO_HAVE_short_load_full)
    template <class T = unsigned short>
    static inline unsigned short load_seq_cst(unsigned short const& var) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_load_full((unsigned short const*)&var);
    }
    #endif

    #if defined(AO_HAVE_short_load_full)
    template <class T = short>
    static inline short load_seq_cst(short const& var) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_load_full((unsigned short const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int load_seq_cst(unsigned int const& var) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_load_full((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int load_seq_cst(int const& var) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_load_full((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_int_load_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int load_seq_cst(unsigned int const& var) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_load_full((unsigned int const*)&var);
    }
    #endif

    #if defined(AO_HAVE_int_load_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int load_seq_cst(int const& var) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_load_full((unsigned int const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_full)
    template <class T = size_t>
    static inline size_t load_seq_cst(size_t const& var) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_load_full((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_full)
    template <class T = ssize_t>
    static inline ssize_t load_seq_cst(ssize_t const& var) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_load_full((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_char_store)
    template <class T = unsigned char>
    static inline void store_relaxed(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        AO_char_store(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_store)
    template <class T = signed char>
    static inline void store_relaxed(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        AO_char_store(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_store)
    template <class T = unsigned short>
    static inline void store_relaxed(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        AO_short_store(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_store)
    template <class T = short>
    static inline void store_relaxed(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        AO_short_store(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_store) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline void store_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_store(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_store) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline void store_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_store(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_store) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline void store_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        AO_int_store(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_store) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline void store_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        AO_int_store(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_store)
    template <class T = size_t>
    static inline void store_relaxed(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_store(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_store)
    template <class T = ssize_t>
    static inline void store_relaxed(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_store(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_store_release)
    template <class T = unsigned char>
    static inline void store_release(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        AO_char_store_release(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_store_release)
    template <class T = signed char>
    static inline void store_release(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        AO_char_store_release(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_store_release)
    template <class T = unsigned short>
    static inline void store_release(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        AO_short_store_release(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_store_release)
    template <class T = short>
    static inline void store_release(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        AO_short_store_release(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_store_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline void store_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_store_release(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_store_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline void store_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_store_release(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_store_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline void store_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        AO_int_store_release(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_store_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline void store_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        AO_int_store_release(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_store_release)
    template <class T = size_t>
    static inline void store_release(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_store_release(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_store_release)
    template <class T = ssize_t>
    static inline void store_release(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_store_release(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_store_full)
    template <class T = unsigned char>
    static inline void store_seq_cst(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        AO_char_store_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_store_full)
    template <class T = signed char>
    static inline void store_seq_cst(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        AO_char_store_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_store_full)
    template <class T = unsigned short>
    static inline void store_seq_cst(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        AO_short_store_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_store_full)
    template <class T = short>
    static inline void store_seq_cst(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        AO_short_store_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_store_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline void store_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_store_full(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_store_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline void store_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_store_full(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_store_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline void store_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        AO_int_store_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_store_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline void store_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        AO_int_store_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_store_full)
    template <class T = size_t>
    static inline void store_seq_cst(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_store_full(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_store_full)
    template <class T = ssize_t>
    static inline void store_seq_cst(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_store_full(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = unsigned char>
    static inline unsigned char exchange_relaxed(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = signed char>
    static inline signed char exchange_relaxed(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = unsigned short>
    static inline unsigned short exchange_relaxed(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = short>
    static inline short exchange_relaxed(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = size_t>
    static inline size_t exchange_relaxed(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = ssize_t>
    static inline ssize_t exchange_relaxed(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = unsigned char>
    static inline unsigned char exchange_acquire(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = signed char>
    static inline signed char exchange_acquire(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = unsigned short>
    static inline unsigned short exchange_acquire(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = short>
    static inline short exchange_acquire(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = size_t>
    static inline size_t exchange_acquire(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = ssize_t>
    static inline ssize_t exchange_acquire(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = unsigned char>
    static inline unsigned char exchange_release(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = signed char>
    static inline signed char exchange_release(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = unsigned short>
    static inline unsigned short exchange_release(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = short>
    static inline short exchange_release(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = size_t>
    static inline size_t exchange_release(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = ssize_t>
    static inline ssize_t exchange_release(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char exchange_acq_rel(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char exchange_acq_rel(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short exchange_acq_rel(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short exchange_acq_rel(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t exchange_acq_rel(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t exchange_acq_rel(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char exchange_seq_cst(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<unsigned char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char exchange_seq_cst(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                (unsigned char)static_cast<signed char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short exchange_seq_cst(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<unsigned short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short exchange_seq_cst(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                (unsigned short)static_cast<short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int exchange_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int exchange_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                (unsigned int)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t exchange_seq_cst(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<size_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t exchange_seq_cst(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<ssize_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = unsigned char>
    static inline bool compare_exchange_relaxed(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = signed char>
    static inline bool compare_exchange_relaxed(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = unsigned short>
    static inline bool compare_exchange_relaxed(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = short>
    static inline bool compare_exchange_relaxed(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = size_t>
    static inline bool compare_exchange_relaxed(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = ssize_t>
    static inline bool compare_exchange_relaxed(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = unsigned char>
    static inline bool compare_exchange_acquire(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_acquire(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = signed char>
    static inline bool compare_exchange_acquire(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_acquire(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = unsigned short>
    static inline bool compare_exchange_acquire(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_acquire(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = short>
    static inline bool compare_exchange_acquire(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_acquire(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acquire(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acquire(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acquire(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_acquire(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acquire(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_acquire(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = size_t>
    static inline bool compare_exchange_acquire(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = ssize_t>
    static inline bool compare_exchange_acquire(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = unsigned char>
    static inline bool compare_exchange_acquire_relaxed(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_acquire(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = signed char>
    static inline bool compare_exchange_acquire_relaxed(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_acquire(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = unsigned short>
    static inline bool compare_exchange_acquire_relaxed(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_acquire(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = short>
    static inline bool compare_exchange_acquire_relaxed(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_acquire(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acquire_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acquire_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acquire_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_acquire(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acquire_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_acquire(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = size_t>
    static inline bool compare_exchange_acquire_relaxed(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = ssize_t>
    static inline bool compare_exchange_acquire_relaxed(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = unsigned char>
    static inline bool compare_exchange_release(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_release(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = signed char>
    static inline bool compare_exchange_release(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_release(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = unsigned short>
    static inline bool compare_exchange_release(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_release(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = short>
    static inline bool compare_exchange_release(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_release(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_release(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_release(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_release(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_release(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_release(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_release(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = size_t>
    static inline bool compare_exchange_release(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = ssize_t>
    static inline bool compare_exchange_release(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = unsigned char>
    static inline bool compare_exchange_release_relaxed(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_release(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = signed char>
    static inline bool compare_exchange_release_relaxed(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_release(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = unsigned short>
    static inline bool compare_exchange_release_relaxed(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_release(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = short>
    static inline bool compare_exchange_release_relaxed(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_release(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_release_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_release_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_release_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_release(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_release_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_release(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = size_t>
    static inline bool compare_exchange_release_relaxed(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = ssize_t>
    static inline bool compare_exchange_release_relaxed(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline bool compare_exchange_acq_rel(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline bool compare_exchange_acq_rel(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline bool compare_exchange_acq_rel(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline bool compare_exchange_acq_rel(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acq_rel(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acq_rel(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acq_rel(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acq_rel(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline bool compare_exchange_acq_rel(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline bool compare_exchange_acq_rel(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline bool compare_exchange_acq_rel_relaxed(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline bool compare_exchange_acq_rel_relaxed(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline bool compare_exchange_acq_rel_relaxed(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline bool compare_exchange_acq_rel_relaxed(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acq_rel_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acq_rel_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_acq_rel_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_acq_rel_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline bool compare_exchange_acq_rel_relaxed(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline bool compare_exchange_acq_rel_relaxed(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline bool compare_exchange_seq_cst(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        AO_nop_full();
        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));
        AO_nop_full();

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline bool compare_exchange_seq_cst(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        AO_nop_full();
        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));
        AO_nop_full();

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline bool compare_exchange_seq_cst(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        AO_nop_full();
        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));
        AO_nop_full();

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline bool compare_exchange_seq_cst(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        AO_nop_full();
        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));
        AO_nop_full();

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_seq_cst(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_nop_full();
        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));
        AO_nop_full();

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_seq_cst(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_nop_full();
        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));
        AO_nop_full();

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_seq_cst(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        AO_nop_full();
        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));
        AO_nop_full();

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_seq_cst(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        AO_nop_full();
        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));
        AO_nop_full();

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline bool compare_exchange_seq_cst(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_nop_full();
        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));
        AO_nop_full();

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline bool compare_exchange_seq_cst(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_nop_full();
        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));
        AO_nop_full();

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline bool compare_exchange_seq_cst_relaxed(unsigned char& var, unsigned char& exp, T des) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<unsigned char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (unsigned char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline bool compare_exchange_seq_cst_relaxed(signed char& var, signed char& exp, T des) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char old = AO_char_fetch_compare_and_swap_full(
            (unsigned char*)&var,
            (unsigned char)exp,
            (unsigned char)static_cast<signed char>(des));

        const bool success = (unsigned char)exp == old;
        exp = (signed char)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline bool compare_exchange_seq_cst_relaxed(unsigned short& var, unsigned short& exp, T des) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<unsigned short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (unsigned short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline bool compare_exchange_seq_cst_relaxed(short& var, short& exp, T des) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short old = AO_short_fetch_compare_and_swap_full(
            (unsigned short*)&var,
            (unsigned short)exp,
            (unsigned short)static_cast<short>(des));

        const bool success = (unsigned short)exp == old;
        exp = (short)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_seq_cst_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<unsigned int>(des));

        const bool success = (AO_t)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_seq_cst_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<int>(des));

        const bool success = (AO_t)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline bool compare_exchange_seq_cst_relaxed(unsigned int& var, unsigned int& exp, T des) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<unsigned int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (unsigned int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline bool compare_exchange_seq_cst_relaxed(int& var, int& exp, T des) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int old = AO_int_fetch_compare_and_swap_full(
            (unsigned int*)&var,
            (unsigned int)exp,
            (unsigned int)static_cast<int>(des));

        const bool success = (unsigned int)exp == old;
        exp = (int)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline bool compare_exchange_seq_cst_relaxed(size_t& var, size_t& exp, T des) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<size_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (size_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline bool compare_exchange_seq_cst_relaxed(ssize_t& var, ssize_t& exp, T des) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<ssize_t>(des));

        const bool success = (AO_t)exp == old;
        exp = (ssize_t)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add)
    template <class T = unsigned char>
    static inline unsigned char fetch_add_relaxed(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add)
    template <class T = signed char>
    static inline signed char fetch_add_relaxed(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add)
    template <class T = unsigned short>
    static inline unsigned short fetch_add_relaxed(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add)
    template <class T = short>
    static inline short fetch_add_relaxed(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add)
    template <class T = size_t>
    static inline size_t fetch_add_relaxed(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add)
    template <class T = ssize_t>
    static inline ssize_t fetch_add_relaxed(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_acquire)
    template <class T = unsigned char>
    static inline unsigned char fetch_add_acquire(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_acquire(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_acquire)
    template <class T = signed char>
    static inline signed char fetch_add_acquire(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_acquire(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_acquire)
    template <class T = unsigned short>
    static inline unsigned short fetch_add_acquire(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_acquire(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_acquire)
    template <class T = short>
    static inline short fetch_add_acquire(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_acquire(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_acquire(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_acquire(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire)
    template <class T = size_t>
    static inline size_t fetch_add_acquire(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire)
    template <class T = ssize_t>
    static inline ssize_t fetch_add_acquire(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_release)
    template <class T = unsigned char>
    static inline unsigned char fetch_add_release(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_release(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_release)
    template <class T = signed char>
    static inline signed char fetch_add_release(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_release(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_release)
    template <class T = unsigned short>
    static inline unsigned short fetch_add_release(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_release(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_release)
    template <class T = short>
    static inline short fetch_add_release(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_release(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_release(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_release(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release)
    template <class T = size_t>
    static inline size_t fetch_add_release(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release)
    template <class T = ssize_t>
    static inline ssize_t fetch_add_release(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_add_acq_rel(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = signed char>
    static inline signed char fetch_add_acq_rel(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_add_acq_rel(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = short>
    static inline short fetch_add_acq_rel(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = size_t>
    static inline size_t fetch_add_acq_rel(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_add_acq_rel(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_add_seq_cst(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = signed char>
    static inline signed char fetch_add_seq_cst(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_add_seq_cst(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = short>
    static inline short fetch_add_seq_cst(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_add_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_add_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = size_t>
    static inline size_t fetch_add_seq_cst(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_add_seq_cst(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add)
    template <class T = unsigned char>
    static inline unsigned char fetch_sub_relaxed(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add)
    template <class T = signed char>
    static inline signed char fetch_sub_relaxed(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add)
    template <class T = unsigned short>
    static inline unsigned short fetch_sub_relaxed(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add)
    template <class T = short>
    static inline short fetch_sub_relaxed(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add)
    template <class T = size_t>
    static inline size_t fetch_sub_relaxed(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add)
    template <class T = ssize_t>
    static inline ssize_t fetch_sub_relaxed(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_acquire)
    template <class T = unsigned char>
    static inline unsigned char fetch_sub_acquire(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_acquire(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_acquire)
    template <class T = signed char>
    static inline signed char fetch_sub_acquire(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_acquire(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_acquire)
    template <class T = unsigned short>
    static inline unsigned short fetch_sub_acquire(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_acquire(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_acquire)
    template <class T = short>
    static inline short fetch_sub_acquire(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_acquire(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_acquire(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_acquire(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire)
    template <class T = size_t>
    static inline size_t fetch_sub_acquire(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire)
    template <class T = ssize_t>
    static inline ssize_t fetch_sub_acquire(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_release)
    template <class T = unsigned char>
    static inline unsigned char fetch_sub_release(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_release(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_release)
    template <class T = signed char>
    static inline signed char fetch_sub_release(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_release(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_release)
    template <class T = unsigned short>
    static inline unsigned short fetch_sub_release(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_release(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_release)
    template <class T = short>
    static inline short fetch_sub_release(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_release(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_release(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_release(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release)
    template <class T = size_t>
    static inline size_t fetch_sub_release(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release)
    template <class T = ssize_t>
    static inline ssize_t fetch_sub_release(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_sub_acq_rel(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = signed char>
    static inline signed char fetch_sub_acq_rel(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_sub_acq_rel(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = short>
    static inline short fetch_sub_acq_rel(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = size_t>
    static inline size_t fetch_sub_acq_rel(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_sub_acq_rel(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_sub_seq_cst(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        return (unsigned char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<unsigned char>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_and_add_full)
    template <class T = signed char>
    static inline signed char fetch_sub_seq_cst(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        return (signed char)AO_char_fetch_and_add_full(
            (unsigned char*)&var,
            (unsigned char)static_cast<signed char>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_sub_seq_cst(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        return (unsigned short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<unsigned short>(-val));
    }
    #endif

    #if defined(AO_HAVE_short_fetch_and_add_full)
    template <class T = short>
    static inline short fetch_sub_seq_cst(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        return (short)AO_short_fetch_and_add_full(
            (unsigned short*)&var,
            (unsigned short)static_cast<short>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        return (unsigned int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        return (int)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_sub_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        return (unsigned int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<unsigned int>(-val));
    }
    #endif

    #if defined(AO_HAVE_int_fetch_and_add_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_sub_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        return (int)AO_int_fetch_and_add_full(
            (unsigned int*)&var,
            (unsigned int)static_cast<int>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = size_t>
    static inline size_t fetch_sub_seq_cst(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        return (size_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<size_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_sub_seq_cst(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        return (ssize_t)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ssize_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = unsigned char>
    static inline unsigned char fetch_and_relaxed(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = signed char>
    static inline signed char fetch_and_relaxed(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = unsigned short>
    static inline unsigned short fetch_and_relaxed(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = short>
    static inline short fetch_and_relaxed(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = size_t>
    static inline size_t fetch_and_relaxed(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = ssize_t>
    static inline ssize_t fetch_and_relaxed(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = unsigned char>
    static inline unsigned char fetch_and_acquire(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = signed char>
    static inline signed char fetch_and_acquire(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = unsigned short>
    static inline unsigned short fetch_and_acquire(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = short>
    static inline short fetch_and_acquire(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = size_t>
    static inline size_t fetch_and_acquire(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = ssize_t>
    static inline ssize_t fetch_and_acquire(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = unsigned char>
    static inline unsigned char fetch_and_release(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = signed char>
    static inline signed char fetch_and_release(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = unsigned short>
    static inline unsigned short fetch_and_release(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = short>
    static inline short fetch_and_release(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = size_t>
    static inline size_t fetch_and_release(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = ssize_t>
    static inline ssize_t fetch_and_release(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_and_acq_rel(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char fetch_and_acq_rel(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_and_acq_rel(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short fetch_and_acq_rel(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t fetch_and_acq_rel(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_and_acq_rel(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_and_seq_cst(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<unsigned char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char fetch_and_seq_cst(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev & (unsigned char)static_cast<signed char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_and_seq_cst(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<unsigned short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short fetch_and_seq_cst(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev & (unsigned short)static_cast<short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_and_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_and_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev & (unsigned int)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t fetch_and_seq_cst(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<size_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_and_seq_cst(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev & (AO_t)static_cast<ssize_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = unsigned char>
    static inline unsigned char fetch_or_relaxed(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = signed char>
    static inline signed char fetch_or_relaxed(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = unsigned short>
    static inline unsigned short fetch_or_relaxed(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = short>
    static inline short fetch_or_relaxed(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = size_t>
    static inline size_t fetch_or_relaxed(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = ssize_t>
    static inline ssize_t fetch_or_relaxed(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = unsigned char>
    static inline unsigned char fetch_or_acquire(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = signed char>
    static inline signed char fetch_or_acquire(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = unsigned short>
    static inline unsigned short fetch_or_acquire(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = short>
    static inline short fetch_or_acquire(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = size_t>
    static inline size_t fetch_or_acquire(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = ssize_t>
    static inline ssize_t fetch_or_acquire(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = unsigned char>
    static inline unsigned char fetch_or_release(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = signed char>
    static inline signed char fetch_or_release(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = unsigned short>
    static inline unsigned short fetch_or_release(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = short>
    static inline short fetch_or_release(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = size_t>
    static inline size_t fetch_or_release(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = ssize_t>
    static inline ssize_t fetch_or_release(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_or_acq_rel(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char fetch_or_acq_rel(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_or_acq_rel(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short fetch_or_acq_rel(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t fetch_or_acq_rel(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_or_acq_rel(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_or_seq_cst(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<unsigned char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char fetch_or_seq_cst(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev | (unsigned char)static_cast<signed char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_or_seq_cst(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<unsigned short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short fetch_or_seq_cst(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev | (unsigned short)static_cast<short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_or_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_or_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev | (unsigned int)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t fetch_or_seq_cst(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<size_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_or_seq_cst(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev | (AO_t)static_cast<ssize_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = unsigned char>
    static inline unsigned char fetch_xor_relaxed(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap)
    template <class T = signed char>
    static inline signed char fetch_xor_relaxed(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = unsigned short>
    static inline unsigned short fetch_xor_relaxed(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap)
    template <class T = short>
    static inline short fetch_xor_relaxed(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_relaxed(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_relaxed(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = size_t>
    static inline size_t fetch_xor_relaxed(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class T = ssize_t>
    static inline ssize_t fetch_xor_relaxed(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = unsigned char>
    static inline unsigned char fetch_xor_acquire(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_acquire)
    template <class T = signed char>
    static inline signed char fetch_xor_acquire(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_acquire(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = unsigned short>
    static inline unsigned short fetch_xor_acquire(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_acquire)
    template <class T = short>
    static inline short fetch_xor_acquire(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_acquire(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_acquire(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_acquire) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_acquire(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_acquire(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = size_t>
    static inline size_t fetch_xor_acquire(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class T = ssize_t>
    static inline ssize_t fetch_xor_acquire(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = unsigned char>
    static inline unsigned char fetch_xor_release(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_release)
    template <class T = signed char>
    static inline signed char fetch_xor_release(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_release(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = unsigned short>
    static inline unsigned short fetch_xor_release(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_release)
    template <class T = short>
    static inline short fetch_xor_release(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_release(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_release(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_release) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_release(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_release(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = size_t>
    static inline size_t fetch_xor_release(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class T = ssize_t>
    static inline ssize_t fetch_xor_release(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_xor_acq_rel(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<unsigned char>(val));
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char fetch_xor_acq_rel(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<signed char>(val));
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_xor_acq_rel(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<unsigned short>(val));
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short fetch_xor_acq_rel(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<short>(val));
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_acq_rel(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<unsigned int>(val));
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_acq_rel(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<int>(val));
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t fetch_xor_acq_rel(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<size_t>(val));
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_xor_acq_rel(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<ssize_t>(val));
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = unsigned char>
    static inline unsigned char fetch_xor_seq_cst(unsigned char& var, T val) {
        static_assert(sizeof(unsigned char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<unsigned char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned char)curr;
    }
    #endif

    #if defined(AO_HAVE_char_fetch_compare_and_swap_full)
    template <class T = signed char>
    static inline signed char fetch_xor_seq_cst(signed char& var, T val) {
        static_assert(sizeof(signed char) == sizeof(unsigned char), "type mismatch");

        unsigned char curr = AO_char_load_full((unsigned char*)&var);
        unsigned char prev;
        do {
            prev = curr;
            curr = AO_char_fetch_compare_and_swap_full(
                (unsigned char*)&var,
                prev,
                prev ^ (unsigned char)static_cast<signed char>(val));
            AO_nop_full();
        } while (curr != prev);

        return (signed char)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = unsigned short>
    static inline unsigned short fetch_xor_seq_cst(unsigned short& var, T val) {
        static_assert(sizeof(unsigned short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<unsigned short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned short)curr;
    }
    #endif

    #if defined(AO_HAVE_short_fetch_compare_and_swap_full)
    template <class T = short>
    static inline short fetch_xor_seq_cst(short& var, T val) {
        static_assert(sizeof(short) == sizeof(unsigned short), "type mismatch");

        unsigned short curr = AO_short_load_full((unsigned short*)&var);
        unsigned short prev;
        do {
            prev = curr;
            curr = AO_short_fetch_compare_and_swap_full(
                (unsigned short*)&var,
                prev,
                prev ^ (unsigned short)static_cast<short>(val));
            AO_nop_full();
        } while (curr != prev);

        return (short)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full) && defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = unsigned int>
    static inline unsigned int fetch_xor_seq_cst(unsigned int& var, T val) {
        static_assert(sizeof(unsigned int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<unsigned int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (unsigned int)curr;
    }
    #endif

    #if defined(AO_HAVE_int_fetch_compare_and_swap_full) && !defined(AO_T_IS_INT)
    template <class T = int>
    static inline int fetch_xor_seq_cst(int& var, T val) {
        static_assert(sizeof(int) == sizeof(unsigned int), "type mismatch");

        unsigned int curr = AO_int_load_full((unsigned int*)&var);
        unsigned int prev;
        do {
            prev = curr;
            curr = AO_int_fetch_compare_and_swap_full(
                (unsigned int*)&var,
                prev,
                prev ^ (unsigned int)static_cast<int>(val));
            AO_nop_full();
        } while (curr != prev);

        return (int)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = size_t>
    static inline size_t fetch_xor_seq_cst(size_t& var, T val) {
        static_assert(sizeof(size_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<size_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (size_t)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class T = ssize_t>
    static inline ssize_t fetch_xor_seq_cst(ssize_t& var, T val) {
        static_assert(sizeof(ssize_t) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                prev ^ (AO_t)static_cast<ssize_t>(val));
            AO_nop_full();
        } while (curr != prev);

        return (ssize_t)curr;
    }
    #endif

    #if defined(AO_HAVE_load)
    template <class P, class T = P*>
    static inline P* load_relaxed(P* const& var) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_load((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_acquire)
    template <class P, class T = P*>
    static inline P* load_acquire(P* const& var) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_load_acquire((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_load_full)
    template <class P, class T = P*>
    static inline P* load_seq_cst(P* const& var) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_load_full((AO_t const*)&var);
    }
    #endif

    #if defined(AO_HAVE_store)
    template <class P, class T = P*>
    static inline void store_relaxed(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_store(
            (AO_t*)&var,
            (AO_t)static_cast<P*>(val));
    }
    #endif

    #if defined(AO_HAVE_store_release)
    template <class P, class T = P*>
    static inline void store_release(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_store_release(
            (AO_t*)&var,
            (AO_t)static_cast<P*>(val));
    }
    #endif

    #if defined(AO_HAVE_store_full)
    template <class P, class T = P*>
    static inline void store_seq_cst(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_store_full(
            (AO_t*)&var,
            (AO_t)static_cast<P*>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class P, class T = P*>
    static inline P* exchange_relaxed(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<P*>(val));
        } while (curr != prev);

        return (P*)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class P, class T = P*>
    static inline P* exchange_acquire(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_acquire(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<P*>(val));
        } while (curr != prev);

        return (P*)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class P, class T = P*>
    static inline P* exchange_release(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_release(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<P*>(val));
        } while (curr != prev);

        return (P*)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class P, class T = P*>
    static inline P* exchange_acq_rel(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<P*>(val));
        } while (curr != prev);

        return (P*)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class P, class T = P*>
    static inline P* exchange_seq_cst(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t curr = AO_load_full((AO_t*)&var);
        AO_t prev;
        do {
            prev = curr;
            curr = AO_fetch_compare_and_swap_full(
                (AO_t*)&var,
                prev,
                (AO_t)static_cast<P*>(val));
            AO_nop_full();
        } while (curr != prev);

        return (P*)curr;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap)
    template <class P, class T = P*>
    static inline bool compare_exchange_relaxed(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class P, class T = P*>
    static inline bool compare_exchange_acquire(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_acquire)
    template <class P, class T = P*>
    static inline bool compare_exchange_acquire_relaxed(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_acquire(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class P, class T = P*>
    static inline bool compare_exchange_release(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_release)
    template <class P, class T = P*>
    static inline bool compare_exchange_release_relaxed(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_release(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class P, class T = P*>
    static inline bool compare_exchange_acq_rel(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class P, class T = P*>
    static inline bool compare_exchange_acq_rel_relaxed(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class P, class T = P*>
    static inline bool compare_exchange_seq_cst(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_nop_full();
        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));
        AO_nop_full();

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_compare_and_swap_full)
    template <class P, class T = P*>
    static inline bool compare_exchange_seq_cst_relaxed(P*& var, P*& exp, T des) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        AO_t old = AO_fetch_compare_and_swap_full(
            (AO_t*)&var,
            (AO_t)exp,
            (AO_t)static_cast<P*>(des));

        const bool success = (AO_t)exp == old;
        exp = (P*)old;

        return success;
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add)
    template <class P, class T = P*>
    static inline P* fetch_add_relaxed(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire)
    template <class P, class T = P*>
    static inline P* fetch_add_acquire(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release)
    template <class P, class T = P*>
    static inline P* fetch_add_release(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class P, class T = P*>
    static inline P* fetch_add_acq_rel(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class P, class T = P*>
    static inline P* fetch_add_seq_cst(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add)
    template <class P, class T = P*>
    static inline P* fetch_sub_relaxed(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_acquire)
    template <class P, class T = P*>
    static inline P* fetch_sub_acquire(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_acquire(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_release)
    template <class P, class T = P*>
    static inline P* fetch_sub_release(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_release(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class P, class T = P*>
    static inline P* fetch_sub_acq_rel(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(-val));
    }
    #endif

    #if defined(AO_HAVE_fetch_and_add_full)
    template <class P, class T = P*>
    static inline P* fetch_sub_seq_cst(P*& var, T val) {
        static_assert(sizeof(P*) == sizeof(AO_t), "type mismatch");

        return (P*)AO_fetch_and_add_full(
            (AO_t*)&var,
            (AO_t)static_cast<ptrdiff_t>(-val));
    }
    #endif
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_OPS_H_
