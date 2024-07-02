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

bool StringList::is_empty() const {
    return size_ == 0;
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

    check_member_(str);

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

    check_member_(str);

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
    front_ = NULL;
    back_ = NULL;
    size_ = 0;
}

bool StringList::push_back(const char* str) {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    return push_back(str, str + strlen(str));
}

bool StringList::push_back(const char* str_begin, const char* str_end) {
    if (str_begin == NULL || str_end == NULL || str_begin > str_end) {
        roc_panic("stringlist: invalid range");
    }

    const size_t str_sz = (size_t)(str_end - str_begin);
    const size_t blk_sz =
        sizeof(Header) + AlignOps::align_as(str_sz + 1, sizeof(Header)) + sizeof(Footer);

    if (!grow_(data_.size() + blk_sz)) {
        return false;
    }

    if (!data_.resize(data_.size() + blk_sz)) {
        return false;
    }

    front_ = (Header*)(data_.data());
    back_ = (Header*)(data_.data() + data_.size() - blk_sz);
    size_++;

    Header* str_header = back_;
    str_header->len = (uint32_t)blk_sz;

    memcpy(str_header->str, str_begin, str_sz); // copy string
    str_header->str[str_sz] = '\0';             // add null

    Footer* str_footer = (Footer*)((char*)back_ + blk_sz - sizeof(Footer));
    str_footer->len = (uint32_t)blk_sz;

    return true;
}

bool StringList::pop_back() {
    if (size_ == 0) {
        roc_panic("stringlist: list is empty");
    }

    const size_t blk_sz = back_->len;
    const Footer* prev_footer = NULL;
    if (size_ > 1) {
        prev_footer = (const Footer*)((const char*)back_ - sizeof(Footer));
    }

    if (!data_.resize(data_.size() - blk_sz)) {
        return false;
    }

    size_--;
    if (size_) {
        back_ = (Header*)(data_.data() + data_.size() - prev_footer->len);
    } else {
        front_ = NULL;
        back_ = NULL;
    }

    return true;
}

const char* StringList::find(const char* str) {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    return find(str, str + strlen(str));
}

const char* StringList::find(const char* str_begin, const char* str_end) {
    if (str_begin == NULL || str_end == NULL || str_begin > str_end) {
        roc_panic("stringlist: invalid range");
    }

    if (size_ != 0) {
        const size_t str_sz = (size_t)(str_end - str_begin);
        const size_t blk_sz = sizeof(Header)
            + AlignOps::align_as(str_sz + 1, sizeof(Header)) + sizeof(Footer);

        const Header* s_header = front_;
        for (;;) {
            if (s_header->len == blk_sz
                && memcmp(s_header->str, str_begin, str_sz) == 0) {
                return s_header->str;
            }
            if (s_header == back_) {
                break;
            }
            s_header = (const Header*)((const char*)s_header + s_header->len);
        }
    }

    return NULL;
}

void StringList::check_member_(const char* str) const {
    if (size_ == 0) {
        roc_panic("stringlist: list is empty");
    }

    const char* begin = &data_[0];
    const char* end = &data_[0] + data_.size();

    if (str < begin || str >= end) {
        roc_panic("stringlist: string doesn't belong to the list");
    }
}

bool StringList::grow_(size_t new_size) {
    if (new_size < MinCapacity) {
        new_size = MinCapacity;
    }

    return data_.grow_exp(new_size);
}

} // namespace core
} // namespace roc
