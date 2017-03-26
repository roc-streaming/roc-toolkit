/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_CORE_TEST_OBJECT_H_
#define ROC_CORE_TEST_OBJECT_H_

#include <cstdlib>
#include <map>

namespace roc {
namespace test {

struct TestObject {
    static std::map<const TestObject*, size_t> state;

    enum { Initialized = 111, Destroyed = 222 };

    size_t value() const {
        return state[this];
    }

    void set_value(size_t v) {
        state[this] = v;
    }

    TestObject(size_t v = Initialized) {
        set_value(v);
    }

    TestObject(const TestObject& other) {
        set_value(other.value());
    }

    ~TestObject() {
        set_value(Destroyed);
    }

    TestObject& operator=(const TestObject& other) {
        set_value(other.value());
        return *this;
    }
};

} // namespace test
} // namespace roc

#endif // ROC_CORE_TEST_OBJECT_H_
