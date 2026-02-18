/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/hashmap.h"
#include "roc_core/hashsum.h"
#include "roc_core/heap_arena.h"
#include "roc_core/noop_arena.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace core {

namespace {

struct TestAllocation {
    virtual ~TestAllocation() {
    }

    void dispose() {
        delete this;
    }
};

class Object : public HashmapNode<>, public RefCounted<Object, TestAllocation> {
public:
    Object(const char* k) {
        strcpy(key_, k);
    }

    static hashsum_t key_hash(const char* key) {
        return hashsum_str(key);
    }

    static bool key_equal(const char* key1, const char* key2) {
        return strcmp(key1, key2) == 0;
    }

    const char* key() const {
        return key_;
    }

private:
    char key_[64];
};

void format_key(char* key, size_t keysz, size_t n) {
    StringBuilder b(key, keysz);
    CHECK(b.append_str("key"));
    CHECK(b.append_uint((uint64_t)n, 10));
    CHECK(b.is_ok());
}

HeapArena arena;

} // namespace

TEST_GROUP(hashmap) {};

TEST(hashmap, empty) {
    Hashmap<Object> hashmap(arena);

    UNSIGNED_LONGS_EQUAL(0, hashmap.size());
    UNSIGNED_LONGS_EQUAL(0, hashmap.capacity());

    UNSIGNED_LONGS_EQUAL(0, arena.num_allocations());
}

TEST(hashmap, insert) {
    SharedPtr<Object> obj = new Object("foo");

    Hashmap<Object> hashmap(arena);
    UNSIGNED_LONGS_EQUAL(0, hashmap.size());

    CHECK(!hashmap.find("foo"));

    CHECK(hashmap.grow());

    CHECK(hashmap.insert(*obj));
    UNSIGNED_LONGS_EQUAL(1, hashmap.size());

    CHECK(hashmap.find("foo") == obj);
}

TEST(hashmap, remove) {
    SharedPtr<Object> obj = new Object("foo");

    Hashmap<Object> hashmap(arena);
    UNSIGNED_LONGS_EQUAL(0, hashmap.size());

    CHECK(!hashmap.find("foo"));

    CHECK(hashmap.grow());

    CHECK(hashmap.insert(*obj));
    UNSIGNED_LONGS_EQUAL(1, hashmap.size());

    CHECK(hashmap.find("foo"));

    hashmap.remove(*obj);
    UNSIGNED_LONGS_EQUAL(0, hashmap.size());

    CHECK(!hashmap.find("foo"));
}

TEST(hashmap, insert_remove_many) {
    enum { NumIterations = 10, NumElements = 200 };

    Hashmap<Object> hashmap(arena);

    for (size_t i = 0; i < NumIterations; i++) {
        UNSIGNED_LONGS_EQUAL(0, hashmap.size());

        for (size_t n = 0; n < NumElements; n++) {
            char key[64];
            format_key(key, sizeof(key), n);

            SharedPtr<Object> obj = new Object(key);

            if (hashmap.size() == hashmap.capacity()) {
                CHECK(hashmap.grow());
                CHECK(hashmap.size() < hashmap.capacity());
            }

            CHECK(hashmap.insert(*obj));
        }

        UNSIGNED_LONGS_EQUAL(NumElements, hashmap.size());

        for (size_t n = 0; n < NumElements; n++) {
            char key[64];
            format_key(key, sizeof(key), n);

            SharedPtr<Object> obj = hashmap.find(key);

            CHECK(obj);
            STRCMP_EQUAL(obj->key(), key);

            hashmap.remove(*obj);
        }
    }
}

TEST(hashmap, grow_rapidly) {
    enum { NumIterations = 5 };

    Hashmap<Object> hashmap(arena);

    UNSIGNED_LONGS_EQUAL(0, hashmap.size());
    UNSIGNED_LONGS_EQUAL(0, hashmap.capacity());
    UNSIGNED_LONGS_EQUAL(0, arena.num_allocations());

    size_t n_elems = 0;

    for (size_t i = 0; i < NumIterations; i++) {
        UNSIGNED_LONGS_EQUAL(n_elems, hashmap.size());

        const size_t old_cap = hashmap.capacity();

        CHECK(hashmap.grow());

        const size_t new_cap = hashmap.capacity();

        CHECK(old_cap < new_cap);
        CHECK(n_elems < new_cap);

        if (i == 0) {
            UNSIGNED_LONGS_EQUAL(1, arena.num_allocations());
        } else {
            UNSIGNED_LONGS_EQUAL(2, arena.num_allocations());
        }

        for (size_t n = old_cap; n < new_cap; n++) {
            char key[64];
            format_key(key, sizeof(key), n_elems++);

            SharedPtr<Object> obj = new Object(key);
            CHECK(hashmap.insert(*obj));

            UNSIGNED_LONGS_EQUAL(n_elems, hashmap.size());
        }
    }
}

TEST(hashmap, grow_rapidly_embedding) {
    enum { NumIterations = 5 };

    Hashmap<Object, 50> hashmap(arena);

    UNSIGNED_LONGS_EQUAL(0, hashmap.size());
    UNSIGNED_LONGS_EQUAL(0, hashmap.capacity());
    UNSIGNED_LONGS_EQUAL(0, arena.num_allocations());

    size_t n_elems = 0;

    for (size_t i = 0; i < NumIterations; i++) {
        const size_t cap = hashmap.capacity();

        while (n_elems < cap) {
            char key[64];
            format_key(key, sizeof(key), n_elems++);

            SharedPtr<Object> obj = new Object(key);
            CHECK(hashmap.insert(*obj));
        }

        UNSIGNED_LONGS_EQUAL(n_elems, hashmap.size());

        if (i < 2) {
            UNSIGNED_LONGS_EQUAL(0, arena.num_allocations());
        } else if (i < 3) {
            UNSIGNED_LONGS_EQUAL(1, arena.num_allocations());
        } else {
            UNSIGNED_LONGS_EQUAL(2, arena.num_allocations());
        }

        CHECK(hashmap.grow());

        const size_t new_cap = hashmap.capacity();

        CHECK(n_elems < new_cap);
    }
}

TEST(hashmap, grow_slowly) {
    enum {
        NumElements = 5000,
        StartSize = 77,
        GrowthRatio = 5 // keep every 5th element
    };

    Hashmap<Object> hashmap(arena);

    for (size_t n = 0; n < NumElements; n++) {
        {
            char key[64];
            format_key(key, sizeof(key), n);

            SharedPtr<Object> obj = new Object(key);

            if (hashmap.size() == hashmap.capacity()) {
                CHECK(hashmap.grow());
                CHECK(hashmap.size() < hashmap.capacity());
            }

            CHECK(hashmap.insert(*obj));
        }

        if (n > StartSize && n % GrowthRatio != 0) {
            char key[64];
            format_key(key, sizeof(key), n - 10);

            SharedPtr<Object> obj = hashmap.find(key);

            CHECK(obj);
            STRCMP_EQUAL(obj->key(), key);

            hashmap.remove(*obj);
        }
    }
}

TEST(hashmap, refcounting) {
    SharedPtr<Object> obj1 = new Object("foo");
    SharedPtr<Object> obj2 = new Object("bar");

    UNSIGNED_LONGS_EQUAL(1, obj1->getref());
    UNSIGNED_LONGS_EQUAL(1, obj2->getref());

    {
        Hashmap<Object> hashmap(arena);

        CHECK(hashmap.grow());

        CHECK(hashmap.insert(*obj1));
        CHECK(hashmap.insert(*obj2));

        UNSIGNED_LONGS_EQUAL(2, obj1->getref());
        UNSIGNED_LONGS_EQUAL(2, obj2->getref());

        hashmap.remove(*obj1);

        UNSIGNED_LONGS_EQUAL(1, obj1->getref());
        UNSIGNED_LONGS_EQUAL(2, obj2->getref());

        {
            SharedPtr<Object> obj3 = hashmap.find("bar");

            UNSIGNED_LONGS_EQUAL(1, obj1->getref());
            UNSIGNED_LONGS_EQUAL(3, obj2->getref());
        }

        UNSIGNED_LONGS_EQUAL(1, obj1->getref());
        UNSIGNED_LONGS_EQUAL(2, obj2->getref());
    }

    UNSIGNED_LONGS_EQUAL(1, obj1->getref());
    UNSIGNED_LONGS_EQUAL(1, obj2->getref());
}

TEST(hashmap, iterate_forward) {
    enum { NumElements = 200 };

    Hashmap<Object> hashmap(arena);

    SharedPtr<Object> objects[NumElements];

    CHECK(!hashmap.front());
    CHECK(!hashmap.back());

    for (size_t n = 0; n < NumElements; n++) {
        char key[64];
        format_key(key, sizeof(key), n);

        SharedPtr<Object> obj = new Object(key);

        CHECK(hashmap.grow());
        CHECK(hashmap.insert(*obj));

        objects[n] = obj;

        CHECK(hashmap.front() == objects[0]);
        CHECK(hashmap.back() == objects[n]);
    }

    size_t pos = 0;

    for (SharedPtr<Object> obj = hashmap.front(); obj; obj = hashmap.nextof(*obj)) {
        CHECK(obj == objects[pos]);
        pos++;
    }

    UNSIGNED_LONGS_EQUAL(NumElements, pos);
}

TEST(hashmap, iterate_backward) {
    enum { NumElements = 200 };

    Hashmap<Object> hashmap(arena);

    SharedPtr<Object> objects[NumElements];

    CHECK(!hashmap.front());
    CHECK(!hashmap.back());

    for (size_t n = 0; n < NumElements; n++) {
        char key[64];
        format_key(key, sizeof(key), n);

        SharedPtr<Object> obj = new Object(key);

        CHECK(hashmap.grow());
        CHECK(hashmap.insert(*obj));

        objects[n] = obj;

        CHECK(hashmap.front() == objects[0]);
        CHECK(hashmap.back() == objects[n]);
    }
}

TEST(hashmap, iterate_modify) {
    enum { NumElements = 200 };

    Hashmap<Object> hashmap(arena);

    SharedPtr<Object> objects[NumElements];

    CHECK(!hashmap.front());
    CHECK(!hashmap.back());

    for (size_t n = 0; n < NumElements - 1; n++) {
        char key[64];
        format_key(key, sizeof(key), n);

        SharedPtr<Object> obj = new Object(key);

        CHECK(hashmap.grow());
        CHECK(hashmap.insert(*obj));

        objects[n] = obj;

        CHECK(hashmap.front() == objects[0]);
        CHECK(hashmap.back() == objects[n]);
    }

    size_t pos = 0;

    for (SharedPtr<Object> obj = hashmap.front(); obj; obj = hashmap.nextof(*obj)) {
        if (pos == 2) {
            // remove already visited element during iteration
            hashmap.remove(*objects[1]);
        }

        if (pos == 3) {
            // insert new element during iteration
            char key[64];
            format_key(key, sizeof(key), NumElements - 1);

            SharedPtr<Object> new_obj = new Object(key);

            CHECK(hashmap.grow());
            CHECK(hashmap.insert(*new_obj));

            objects[NumElements - 1] = new_obj;
        }

        CHECK(obj == objects[pos]);
        pos++;
    }

    UNSIGNED_LONGS_EQUAL(NumElements, pos);
}

template <size_t Capacity> void test_embedded_capacity() {
    Hashmap<Object, Capacity> hashmap(core::NoopArena);

    UNSIGNED_LONGS_EQUAL(0, hashmap.capacity());

    size_t n = 0;

    for (;;) {
        if (!hashmap.grow()) {
            break;
        }
        n++;

        char key[64];
        format_key(key, sizeof(key), n);

        SharedPtr<Object> obj = new Object(key);
        CHECK(hashmap.insert(*obj));
    }

    CHECK((ssize_t)n >= (ssize_t)Capacity);
}

TEST(hashmap, embedded_capacity) {
    test_embedded_capacity<0>();
    test_embedded_capacity<5>();
    test_embedded_capacity<10>();
    test_embedded_capacity<15>();
    test_embedded_capacity<20>();
    test_embedded_capacity<25>();
    test_embedded_capacity<30>();
    test_embedded_capacity<35>();
    test_embedded_capacity<40>();
    test_embedded_capacity<45>();
    test_embedded_capacity<50>();
    test_embedded_capacity<55>();
    test_embedded_capacity<60>();
    test_embedded_capacity<65>();
    test_embedded_capacity<70>();
    test_embedded_capacity<75>();
    test_embedded_capacity<80>();
    test_embedded_capacity<85>();
    test_embedded_capacity<90>();
    test_embedded_capacity<95>();
    test_embedded_capacity<100>();
}

} // namespace core
} // namespace roc
