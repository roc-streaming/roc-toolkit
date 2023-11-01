/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/string_list.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

StringList::StringList(IArena& arena)
    : data_(arena)
    , front_(NULL)
    , back_(NULL)
    , size_(0) {
}

size_t StringList::size() const {
    return size_;
}

const char* StringList::front() const {
    if (size_) {
        return front_->str;
    } else {
        return NULL;
    }
}

const char* StringList::back() const {
    if (size_) {
        return back_->str;
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

    const Header* str_header = ROC_CONTAINER_OF(const_cast<char*>(str), Header, str);

    if (str_header == back_) {
        return NULL;
    }

    const Header* next_header =
        (const Header*)((const char*)str_header + str_header->len);
    return next_header->str;
}

const char* StringList::prevof(const char* str) const {
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

    const Header* str_header = ROC_CONTAINER_OF(const_cast<char*>(str), Header, str);

    if (str_header == front_) {
        return NULL;
    }

    const Footer* prev_footer = (const Footer*)((const char*)str_header - sizeof(Footer));
    const Header* prev_header =
        (const Header*)((const char*)str_header - prev_footer->len);

    return prev_header->str;
}

void StringList::clear() {
    data_.clear();
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
    const size_t str_sz = (size_t)(end - begin) + 1;
    const size_t add_sz =
        sizeof(Header) + AlignOps::align_as(str_sz, sizeof(Header)) + sizeof(Footer);

    if (!grow_(cur_sz + add_sz)) {
        return false;
    }

    if (!data_.resize(cur_sz + add_sz)) {
        return false;
    }

    front_ = (Header*)(data_.data());
    back_ = (Header*)(data_.data() + cur_sz);

    Header* str_header = back_;
    str_header->len = static_cast<uint32_t>(add_sz);

    memcpy(&data_[cur_sz + sizeof(Header)], begin, str_sz - 1); // copy string
    data_[cur_sz + sizeof(Header) + str_sz - 1] = '\0';         // add null

    Footer* str_footer = (Footer*)(data_.data() + cur_sz + add_sz - sizeof(Footer));
    str_footer->len = static_cast<uint32_t>(add_sz);
    size_++;

    return true;
}

// bool StringList::push_unique(const char* str) {
//     if (str == NULL) {
//         roc_panic("stringlist: string is null");
//     }

//     return push_unique(str, str + strlen(str));
// }

// bool StringList::push_unique(const char* begin, const char* end) {
//     if (begin == NULL || end == NULL || begin > end) {
//         roc_panic("stringlist: invalid range");
//     }

//     for (const char* s = front(); s; s = nextof(s)) {
//         const size_t s_len = strlen(s);
//         if (s_len == size_t(end - begin) && memcmp(s, begin, s_len) == 0) {
//             return true;
//         }
//     }

//     return push_back(begin, end);
// }

const char* StringList::find(const char* str) {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    return find(str, str + strlen(str));
}

const char* StringList::find(const char* begin, const char* end) {
    if (begin == NULL || end == NULL || begin > end) {
        roc_panic("stringlist: invalid range");
    }

    const size_t str_len = (size_t)(end - begin);
    const size_t find_sz =
        sizeof(Header) + AlignOps::align_as(str_len + 1, sizeof(Header)) + sizeof(Footer);
    for (const char* s = front(); s; s = nextof(s)) {
        const Header* s_header = ROC_CONTAINER_OF(const_cast<char*>(s), Header, str);
        if (s_header->len == find_sz && memcmp(s, begin, str_len) == 0) {
            return s;
        }
    }

    return NULL;
}

bool StringList::grow_(size_t new_size) {
    if (new_size < MinCapacity) {
        new_size = MinCapacity;
    }

    return data_.grow_exp(new_size);
}

} // namespace core
} // namespace roc
