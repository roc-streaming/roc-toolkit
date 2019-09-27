/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/string_list.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

StringList::StringList(IAllocator& allocator)
    : data_(allocator)
    , size_(0) {
}

size_t StringList::size() const {
    return size_;
}

const char* StringList::front() const {
    if (size_) {
        return &data_[0];
    } else {
        return NULL;
    }
}

const char* StringList::nextof(const char* str) const {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    if (size_ == 0) {
        roc_panic("stringlist: list is empty");
    }

    const char* begin = &data_[0];
    const char* end = &data_[0] + data_.size();

    if (str < begin || str >= end) {
        roc_panic("stringlist: string doesn't belong to the list");
    }

    const char* ptr = str + strlen(str) + 1;
    roc_panic_if(ptr > end);

    if (ptr == end) {
        return NULL;
    }

    return ptr;
}

bool StringList::push_back(const char* str) {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    const size_t cur_sz = data_.size();
    const size_t add_sz = strlen(str) + 1;

    if (!grow_(cur_sz + add_sz)) {
        return false;
    }

    if (!data_.resize(cur_sz + add_sz)) {
        return false;
    }

    memcpy(&data_[cur_sz], str, add_sz);
    size_++;

    return true;
}

bool StringList::push_back_uniq(const char* str) {
    for (const char* s = front(); s; s = nextof(s)) {
        if (strcmp(s, str) == 0) {
            return true;
        }
    }
    return push_back(str);
}

void StringList::clear() {
    data_.resize(0);
    size_ = 0;
}

bool StringList::grow_(size_t new_size) {
    if (new_size <= data_.max_size()) {
        return true;
    }

    size_t new_capacity = data_.size();

    if (new_capacity < MinCapacity) {
        new_capacity = MinCapacity;
    }

    while (new_capacity < new_size) {
        new_capacity *= 2;
    }

    return data_.grow(new_capacity);
}

} // namespace core
} // namespace roc
