/*
 * Copyright (c) 2019 Roc Streaming authors
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
    , back_(NULL)
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

const char* StringList::back() const {
    if (size_) {
        return back_;
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

void StringList::clear() {
    data_.resize(0);
    back_ = NULL;
    size_ = 0;
}

bool StringList::push_back(const char* str) {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    return push_back(str, str + strlen(str));
}

bool StringList::push_back(const char* begin, const char* end) {
    if (begin == NULL || end == NULL || begin > end) {
        roc_panic("stringlist: invalid range");
    }

    const size_t cur_sz = data_.size();
    const size_t add_sz = (size_t)(end - begin) + 1;

    if (!grow_(cur_sz + add_sz)) {
        return false;
    }

    if (!data_.resize(cur_sz + add_sz)) {
        return false;
    }

    back_ = &data_[cur_sz];
    memcpy(&data_[cur_sz], begin, add_sz - 1);
    data_[cur_sz + add_sz - 1] = '\0';
    size_++;

    return true;
}

bool StringList::push_unique(const char* str) {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    return push_unique(str, str + strlen(str));
}

bool StringList::push_unique(const char* begin, const char* end) {
    if (begin == NULL || end == NULL || begin > end) {
        roc_panic("stringlist: invalid range");
    }

    for (const char* s = front(); s; s = nextof(s)) {
        const size_t s_len = strlen(s);
        if (s_len == size_t(end - begin) && memcmp(s, begin, s_len) == 0) {
            return true;
        }
    }

    return push_back(begin, end);
}

bool StringList::grow_(size_t new_size) {
    if (new_size < MinCapacity) {
        new_size = MinCapacity;
    }

    return data_.grow_exp(new_size);
}

} // namespace core
} // namespace roc
