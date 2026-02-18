/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/atomic_ops.h"
#include "roc_core/cpu_traits.h"

namespace roc {
namespace core {

TEST_GROUP(atomic_ops) {};

TEST(atomic_ops, fence) {
    AtomicOps::fence_acquire();
    AtomicOps::fence_release();
    AtomicOps::fence_seq_cst();
}

TEST(atomic_ops, load) {
    { // load_relaxed
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::load_relaxed(var));
    }

    { // load_acquire
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::load_acquire(var));
    }

    { // load_seq_cst
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::load_seq_cst(var));
    }
}

TEST(atomic_ops, store) {
    { // store_relaxed
        int var = 10;
        AtomicOps::store_relaxed(var, 20);
        LONGS_EQUAL(20, var);
    }

    { // store_release
        int var = 10;
        AtomicOps::store_release(var, 20);
        LONGS_EQUAL(20, var);
    }

    { // store_seq_cst
        int var = 10;
        AtomicOps::store_seq_cst(var, 20);
        LONGS_EQUAL(20, var);
    }
}

TEST(atomic_ops, exchange) {
    { // exchange_relaxed
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::exchange_relaxed(var, 20));
        LONGS_EQUAL(20, var);
    }

    { // exchange_acquire
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::exchange_acquire(var, 20));
        LONGS_EQUAL(20, var);
    }

    { // exchange_release
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::exchange_release(var, 20));
        LONGS_EQUAL(20, var);
    }

    { // exchange_acq_rel
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::exchange_acq_rel(var, 20));
        LONGS_EQUAL(20, var);
    }

    { // exchange_seq_cst
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::exchange_seq_cst(var, 20));
        LONGS_EQUAL(20, var);
    }
}

TEST(atomic_ops, compare_exchange_success) {
    { // compare_exchange_relaxed
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_relaxed(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_acquire
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_acquire(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_acquire_relaxed
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_acquire_relaxed(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_release
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_release(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_release_relaxed
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_release_relaxed(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_acq_rel
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_acq_rel(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_acq_rel_relaxed
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_acq_rel_relaxed(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_seq_cst
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_seq_cst(var, expected, 20));
        LONGS_EQUAL(20, var);
    }

    { // compare_exchange_seq_cst_relaxed
        int var = 10;
        int expected = 10;
        CHECK(AtomicOps::compare_exchange_seq_cst_relaxed(var, expected, 20));
        LONGS_EQUAL(20, var);
    }
}

TEST(atomic_ops, compare_exchange_failure) {
    { // compare_exchange_relaxed
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_relaxed(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_acquire
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_acquire(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_acquire_relaxed
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_acquire_relaxed(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_release
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_release(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_release_relaxed
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_release_relaxed(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_acq_rel
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_acq_rel(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_acq_rel_relaxed
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_acq_rel_relaxed(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_seq_cst
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_seq_cst(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }

    { // compare_exchange_seq_cst_relaxed
        int var = 10;
        int expected = 5;
        CHECK_FALSE(AtomicOps::compare_exchange_seq_cst_relaxed(var, expected, 20));
        LONGS_EQUAL(10, var);
        LONGS_EQUAL(10, expected);
    }
}

TEST(atomic_ops, fetch_add) {
    { // fetch_add_relaxed
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::fetch_add_relaxed(var, 20));
        LONGS_EQUAL(30, var);
    }

    { // fetch_add_acquire
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::fetch_add_acquire(var, 20));
        LONGS_EQUAL(30, var);
    }

    { // fetch_add_release
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::fetch_add_release(var, 20));
        LONGS_EQUAL(30, var);
    }

    { // fetch_add_acq_rel
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::fetch_add_acq_rel(var, 20));
        LONGS_EQUAL(30, var);
    }

    { // fetch_add_seq_cst
        int var = 10;
        LONGS_EQUAL(10, AtomicOps::fetch_add_seq_cst(var, 20));
        LONGS_EQUAL(30, var);
    }
}

TEST(atomic_ops, fetch_sub) {
    { // fetch_sub_relaxed
        int var = 30;
        LONGS_EQUAL(30, AtomicOps::fetch_sub_relaxed(var, 10));
        LONGS_EQUAL(20, var);
    }

    { // fetch_sub_acquire
        int var = 30;
        LONGS_EQUAL(30, AtomicOps::fetch_sub_acquire(var, 10));
        LONGS_EQUAL(20, var);
    }

    { // fetch_sub_release
        int var = 30;
        LONGS_EQUAL(30, AtomicOps::fetch_sub_release(var, 10));
        LONGS_EQUAL(20, var);
    }

    { // fetch_sub_acq_rel
        int var = 30;
        LONGS_EQUAL(30, AtomicOps::fetch_sub_acq_rel(var, 10));
        LONGS_EQUAL(20, var);
    }

    { // fetch_sub_seq_cst
        int var = 30;
        LONGS_EQUAL(30, AtomicOps::fetch_sub_seq_cst(var, 10));
        LONGS_EQUAL(20, var);
    }
}

TEST(atomic_ops, fetch_and) {
    { // fetch_and_relaxed
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_and_relaxed(var, 0xff00));
        LONGS_EQUAL(0x0f00, var);
    }

    { // fetch_and_acquire
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_and_acquire(var, 0xff00));
        LONGS_EQUAL(0x0f00, var);
    }

    { // fetch_and_release
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_and_release(var, 0xff00));
        LONGS_EQUAL(0x0f00, var);
    }

    { // fetch_and_acq_rel
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_and_acq_rel(var, 0xff00));
        LONGS_EQUAL(0x0f00, var);
    }

    { // fetch_and_seq_cst
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_and_seq_cst(var, 0xff00));
        LONGS_EQUAL(0x0f00, var);
    }
}

TEST(atomic_ops, fetch_or) {
    { // fetch_or_relaxed
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_or_relaxed(var, 0xff00));
        LONGS_EQUAL(0xfff0, var);
    }

    { // fetch_or_acquire
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_or_acquire(var, 0xff00));
        LONGS_EQUAL(0xfff0, var);
    }

    { // fetch_or_release
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_or_release(var, 0xff00));
        LONGS_EQUAL(0xfff0, var);
    }

    { // fetch_or_acq_rel
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_or_acq_rel(var, 0xff00));
        LONGS_EQUAL(0xfff0, var);
    }

    { // fetch_or_seq_cst
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_or_seq_cst(var, 0xff00));
        LONGS_EQUAL(0xfff0, var);
    }
}

TEST(atomic_ops, fetch_xor) {
    { // fetch_xor_relaxed
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_xor_relaxed(var, 0xff00));
        LONGS_EQUAL(0xf0f0, var);
    }

    { // fetch_xor_acquire
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_xor_acquire(var, 0xff00));
        LONGS_EQUAL(0xf0f0, var);
    }

    { // fetch_xor_release
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_xor_release(var, 0xff00));
        LONGS_EQUAL(0xf0f0, var);
    }

    { // fetch_xor_acq_rel
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_xor_acq_rel(var, 0xff00));
        LONGS_EQUAL(0xf0f0, var);
    }

    { // fetch_xor_seq_cst
        int var = 0x0ff0;
        LONGS_EQUAL(0x0ff0, AtomicOps::fetch_xor_seq_cst(var, 0xff00));
        LONGS_EQUAL(0xf0f0, var);
    }
}

template <class T> static void test_any_type() {
    T var = T();
    T exp = T();
    T arg = T();

    // load
    AtomicOps::load_relaxed(var);
    AtomicOps::load_acquire(var);
    AtomicOps::load_seq_cst(var);

    // store
    AtomicOps::store_relaxed(var, arg);
    AtomicOps::store_release(var, arg);
    AtomicOps::store_seq_cst(var, arg);

    // exchange
    AtomicOps::exchange_relaxed(var, arg);
    AtomicOps::exchange_acquire(var, arg);
    AtomicOps::exchange_release(var, arg);
    AtomicOps::exchange_acq_rel(var, arg);
    AtomicOps::exchange_seq_cst(var, arg);

    // compare_exchange
    AtomicOps::compare_exchange_relaxed(var, exp, arg);
    AtomicOps::compare_exchange_acquire(var, exp, arg);
    AtomicOps::compare_exchange_acquire_relaxed(var, exp, arg);
    AtomicOps::compare_exchange_release(var, exp, arg);
    AtomicOps::compare_exchange_release_relaxed(var, exp, arg);
    AtomicOps::compare_exchange_acq_rel(var, exp, arg);
    AtomicOps::compare_exchange_acq_rel_relaxed(var, exp, arg);
    AtomicOps::compare_exchange_seq_cst(var, exp, arg);
    AtomicOps::compare_exchange_seq_cst_relaxed(var, exp, arg);
}

template <class T> static void test_int_type() {
    // checks common for all types
    test_any_type<T>();

    T var = T();
    T arg = T();

    // fetch_add
    AtomicOps::fetch_add_relaxed(var, arg);
    AtomicOps::fetch_add_acquire(var, arg);
    AtomicOps::fetch_add_release(var, arg);
    AtomicOps::fetch_add_acq_rel(var, arg);
    AtomicOps::fetch_add_seq_cst(var, arg);

    // fetch_sub
    AtomicOps::fetch_sub_relaxed(var, arg);
    AtomicOps::fetch_sub_acquire(var, arg);
    AtomicOps::fetch_sub_release(var, arg);
    AtomicOps::fetch_sub_acq_rel(var, arg);
    AtomicOps::fetch_sub_seq_cst(var, arg);

    // fetch_and
    AtomicOps::fetch_and_relaxed(var, arg);
    AtomicOps::fetch_and_acquire(var, arg);
    AtomicOps::fetch_and_release(var, arg);
    AtomicOps::fetch_and_acq_rel(var, arg);
    AtomicOps::fetch_and_seq_cst(var, arg);

    // fetch_or
    AtomicOps::fetch_or_relaxed(var, arg);
    AtomicOps::fetch_or_acquire(var, arg);
    AtomicOps::fetch_or_release(var, arg);
    AtomicOps::fetch_or_acq_rel(var, arg);
    AtomicOps::fetch_or_seq_cst(var, arg);

    // fetch_xor
    AtomicOps::fetch_xor_relaxed(var, arg);
    AtomicOps::fetch_xor_acquire(var, arg);
    AtomicOps::fetch_xor_release(var, arg);
    AtomicOps::fetch_xor_acq_rel(var, arg);
    AtomicOps::fetch_xor_seq_cst(var, arg);
}

template <class T> static void test_ptr_type() {
    // checks common for all types
    test_any_type<T>();

    T var = T();
    ptrdiff_t arg = 0;

    // fetch_add
    AtomicOps::fetch_add_relaxed(var, arg);
    AtomicOps::fetch_add_acquire(var, arg);
    AtomicOps::fetch_add_release(var, arg);
    AtomicOps::fetch_add_acq_rel(var, arg);
    AtomicOps::fetch_add_seq_cst(var, arg);

    // fetch_sub
    AtomicOps::fetch_sub_relaxed(var, arg);
    AtomicOps::fetch_sub_acquire(var, arg);
    AtomicOps::fetch_sub_release(var, arg);
    AtomicOps::fetch_sub_acq_rel(var, arg);
    AtomicOps::fetch_sub_seq_cst(var, arg);
}

TEST(atomic_ops, int_types) {
    test_int_type<int8_t>();
    test_int_type<uint8_t>();

    test_int_type<int16_t>();
    test_int_type<uint16_t>();

    test_int_type<int32_t>();
    test_int_type<uint32_t>();

#if ROC_CPU_BITS == 64
    test_int_type<int64_t>();
    test_int_type<uint64_t>();
#endif
}

TEST(atomic_ops, ptr_types) {
    test_ptr_type<char*>();
    test_ptr_type<void*>();
}

} // namespace core
} // namespace roc
