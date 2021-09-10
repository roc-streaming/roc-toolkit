/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/hash.h"
#include "roc_core/hashmap.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/ref_counter.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace core {

namespace {

class Object : public HashmapNode, public RefCounter<Object> {
public:
    Object(const char* k) {
        strcpy(key_, k);
        visited_ = false;
    }

    static hash_t key_hash(const char* key) {
        return hash_str(key);
    }

    static bool key_equal(const char* key1, const char* key2) {
        return strcmp(key1, key2) == 0;
    }

    const char* key() const {
        return key_;
    }

    bool is_visited() {
        return visited_;
    }

    void set_visited() {
        visited_ = true;
    }

private:
    friend class RefCounter<Object>;

    void destroy() {
        delete this;
    }

    char key_[64];
    bool visited_;
};

} // namespace

TEST_GROUP(hashmap) {
    HeapAllocator allocator;

    void format_key(char* key, size_t keysz, size_t n) {
        StringBuilder b(key, keysz);
        CHECK(b.append_str("key"));
        CHECK(b.append_uint((uint64_t)n, 10));
        CHECK(b.ok());
    }
};

TEST(hashmap, empty) {
    Hashmap<Object> hashmap(allocator);

    UNSIGNED_LONGS_EQUAL(0, hashmap.size());
    UNSIGNED_LONGS_EQUAL(0, hashmap.capacity());

    UNSIGNED_LONGS_EQUAL(0, allocator.num_allocations());
}

TEST(hashmap, insert) {
    SharedPtr<Object> obj = new Object("foo");

    Hashmap<Object> hashmap(allocator);
    UNSIGNED_LONGS_EQUAL(0, hashmap.size());

    CHECK(!hashmap.find("foo"));

    CHECK(hashmap.grow());

    hashmap.insert(*obj);
    UNSIGNED_LONGS_EQUAL(1, hashmap.size());

    CHECK(hashmap.find("foo") == obj);
}

TEST(hashmap, remove) {
    SharedPtr<Object> obj = new Object("foo");

    Hashmap<Object> hashmap(allocator);
    UNSIGNED_LONGS_EQUAL(0, hashmap.size());

    CHECK(!hashmap.find("foo"));

    CHECK(hashmap.grow());

    hashmap.insert(*obj);
    UNSIGNED_LONGS_EQUAL(1, hashmap.size());

    CHECK(hashmap.find("foo"));

    hashmap.remove(*obj);
    UNSIGNED_LONGS_EQUAL(0, hashmap.size());

    CHECK(!hashmap.find("foo"));
}

TEST(hashmap, insert_remove_many) {
    enum { NumIterations = 10, NumElements = 200 };

    Hashmap<Object> hashmap(allocator);

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

            hashmap.insert(*obj);
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

    Hashmap<Object> hashmap(allocator);

    UNSIGNED_LONGS_EQUAL(0, hashmap.size());
    UNSIGNED_LONGS_EQUAL(0, hashmap.capacity());
    UNSIGNED_LONGS_EQUAL(0, allocator.num_allocations());

    size_t n_elems = 0;

    for (size_t i = 0; i < NumIterations; i++) {
        UNSIGNED_LONGS_EQUAL(n_elems, hashmap.size());

        const size_t old_cap = hashmap.capacity();

        CHECK(hashmap.grow());

        const size_t new_cap = hashmap.capacity();

        CHECK(old_cap < new_cap);
        CHECK(n_elems < new_cap);

        if (i == 0) {
            UNSIGNED_LONGS_EQUAL(1, allocator.num_allocations());
        } else {
            UNSIGNED_LONGS_EQUAL(2, allocator.num_allocations());
        }

        for (size_t n = old_cap; n < new_cap; n++) {
            char key[64];
            format_key(key, sizeof(key), n_elems++);

            SharedPtr<Object> obj = new Object(key);
            hashmap.insert(*obj);

            UNSIGNED_LONGS_EQUAL(n_elems, hashmap.size());
        }
    }
}

TEST(hashmap, grow_rapidly_embedding) {
    enum { NumIterations = 5 };

    Hashmap<Object, 50> hashmap(allocator);

    UNSIGNED_LONGS_EQUAL(0, hashmap.size());
    UNSIGNED_LONGS_EQUAL(0, allocator.num_allocations());

    CHECK(hashmap.capacity() > 0);

    size_t n_elems = 0;

    for (size_t i = 0; i < NumIterations; i++) {
        const size_t cap = hashmap.capacity();

        while (n_elems < cap) {
            char key[64];
            format_key(key, sizeof(key), n_elems++);

            SharedPtr<Object> obj = new Object(key);
            hashmap.insert(*obj);
        }

        UNSIGNED_LONGS_EQUAL(n_elems, hashmap.size());

        if (i == 0) {
            UNSIGNED_LONGS_EQUAL(0, allocator.num_allocations());
        } else if (i == 1) {
            UNSIGNED_LONGS_EQUAL(1, allocator.num_allocations());
        } else {
            UNSIGNED_LONGS_EQUAL(2, allocator.num_allocations());
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

    Hashmap<Object> hashmap(allocator);

    for (size_t n = 0; n < NumElements; n++) {
        {
            char key[64];
            format_key(key, sizeof(key), n);

            SharedPtr<Object> obj = new Object(key);

            if (hashmap.size() == hashmap.capacity()) {
                CHECK(hashmap.grow());
                CHECK(hashmap.size() < hashmap.capacity());
            }

            hashmap.insert(*obj);
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
        Hashmap<Object> hashmap(allocator);

        CHECK(hashmap.grow());

        hashmap.insert(*obj1);
        hashmap.insert(*obj2);

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

} // namespace core
} // namespace roc
