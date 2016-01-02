/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/aligned_storage.h"

namespace roc {
namespace test {

using namespace core;

namespace {

struct Dummy {};

template <class T> struct AlignAs {
    T t;

    AlignAs()
        : t() {
    }
};

template <class T> struct AlignOf {
    struct Padded {
        char c;
        T t;
    };
    enum { value = sizeof(Padded) - sizeof(T) };
};

} // namespace

TEST_GROUP(aligned_storage) {
    void check_compatible(size_t align1, size_t align2){ CHECK(align1 <= align2);
        CHECK(align2 % align1 == 0);
    }
};

TEST(aligned_storage, getters) {
    AlignedStorage<short> as;

    CHECK(as.mem() == (unsigned char*)&as.ref());
    CHECK(&as == &AlignedStorage<short>::container_of(as.ref()));
}

TEST(aligned_storage, size_of) {
    LONGS_EQUAL(sizeof(char), //
                sizeof(AlignedStorage<char>));

    LONGS_EQUAL(sizeof(AlignAs<char>), //
                sizeof(AlignedStorage<AlignAs<char> >));

    LONGS_EQUAL(sizeof(short), //
                sizeof(AlignedStorage<short>));

    LONGS_EQUAL(sizeof(AlignAs<short>), //
                sizeof(AlignedStorage<AlignAs<short> >));

    LONGS_EQUAL(sizeof(int), //
                sizeof(AlignedStorage<int>));

    LONGS_EQUAL(sizeof(AlignAs<int>), //
                sizeof(AlignedStorage<AlignAs<int> >));

    LONGS_EQUAL(sizeof(long), //
                sizeof(AlignedStorage<long>));

    LONGS_EQUAL(sizeof(AlignAs<long>), //
                sizeof(AlignedStorage<AlignAs<long> >));

    LONGS_EQUAL(sizeof(float), //
                sizeof(AlignedStorage<float>));

    LONGS_EQUAL(sizeof(AlignAs<float>), //
                sizeof(AlignedStorage<AlignAs<float> >));

    LONGS_EQUAL(sizeof(double), //
                sizeof(AlignedStorage<double>));

    LONGS_EQUAL(sizeof(AlignAs<double>), //
                sizeof(AlignedStorage<AlignAs<double> >));

    LONGS_EQUAL(sizeof(void (*)()), //
                sizeof(AlignedStorage<void (*)()>));

    LONGS_EQUAL(sizeof(AlignAs<void (*)()>), //
                sizeof(AlignedStorage<AlignAs<void (*)()> >));

    LONGS_EQUAL(sizeof(void (Dummy::*)()), //
                sizeof(AlignedStorage<void (Dummy::*)()>));

    LONGS_EQUAL(sizeof(AlignAs<void (Dummy::*)()>), //
                sizeof(AlignedStorage<AlignAs<void (Dummy::*)()> >));
}

TEST(aligned_storage, align_of) {
    check_compatible(AlignOf<char>::value, //
                     AlignOf<AlignedStorage<char> >::value);

    check_compatible(AlignOf<AlignAs<char> >::value, //
                     AlignOf<AlignedStorage<AlignAs<char> > >::value);

    check_compatible(AlignOf<short>::value, //
                     AlignOf<AlignedStorage<short> >::value);

    check_compatible(AlignOf<AlignAs<short> >::value, //
                     AlignOf<AlignedStorage<AlignAs<short> > >::value);

    check_compatible(AlignOf<int>::value, //
                     AlignOf<AlignedStorage<int> >::value);

    check_compatible(AlignOf<AlignAs<int> >::value, //
                     AlignOf<AlignedStorage<AlignAs<int> > >::value);

    check_compatible(AlignOf<long>::value, //
                     AlignOf<AlignedStorage<long> >::value);

    check_compatible(AlignOf<AlignAs<long> >::value, //
                     AlignOf<AlignedStorage<AlignAs<long> > >::value);

    check_compatible(AlignOf<float>::value, //
                     AlignOf<AlignedStorage<float> >::value);

    check_compatible(AlignOf<AlignAs<float> >::value, //
                     AlignOf<AlignedStorage<AlignAs<float> > >::value);

    check_compatible(AlignOf<double>::value, //
                     AlignOf<AlignedStorage<double> >::value);

    check_compatible(AlignOf<AlignAs<double> >::value, //
                     AlignOf<AlignedStorage<AlignAs<double> > >::value);

    check_compatible(AlignOf<void (*)()>::value, //
                     AlignOf<AlignedStorage<void (*)()> >::value);

    check_compatible(AlignOf<AlignAs<void (*)()> >::value, //
                     AlignOf<AlignedStorage<AlignAs<void (*)()> > >::value);

    check_compatible(AlignOf<void (Dummy::*)()>::value, //
                     AlignOf<AlignedStorage<void (Dummy::*)()> >::value);

    check_compatible(AlignOf<AlignAs<void (Dummy::*)()> >::value, //
                     AlignOf<AlignedStorage<AlignAs<void (Dummy::*)()> > >::value);
}

} // namespace test
} // namespace roc
