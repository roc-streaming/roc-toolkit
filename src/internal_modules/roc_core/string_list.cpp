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

namespace {

int strcmp_lexical(const char* a, const char* b) {
    return strcmp(a, b);
}

int strcmp_natural(const char* a, const char* b) {
    while (*a && *b) {
        if (isdigit(*a) && isdigit(*b)) {
            const long ia = strtol(a, const_cast<char**>((const char**)&a), 10);
            const long ib = strtol(b, const_cast<char**>((const char**)&b), 10);
            if (ia != ib) {
                return ia < ib ? -1 : 1;
            }
        } else {
            if (*a != *b) {
                return *a < *b ? -1 : 1;
            }
            a++;
            b++;
        }
    }
    return *a < *b ? -1 : *a != *b;
}

} // namespace

StringList::StringList(IArena& arena)
    : memory_(arena)
    , head_off_(0)
    , tail_off_(0)
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
        return from_offset_(head_off_)->str;
    } else {
        return NULL;
    }
}

const char* StringList::back() const {
    if (size_) {
        return from_offset_(tail_off_)->str;
    } else {
        return NULL;
    }
}

const char* StringList::nextof(const char* str) const {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    check_member_(str);

    const Header* curr_header = ROC_CONTAINER_OF(const_cast<char*>(str), Header, str);
    if (curr_header == from_offset_(tail_off_)) {
        return NULL;
    }

    const Header* next_header = from_offset_(curr_header->next_off);
    return next_header->str;
}

const char* StringList::prevof(const char* str) const {
    if (str == NULL) {
        roc_panic("stringlist: string is null");
    }

    check_member_(str);

    const Header* curr_header = ROC_CONTAINER_OF(const_cast<char*>(str), Header, str);
    if (curr_header == from_offset_(head_off_)) {
        return NULL;
    }

    const Header* prev_header = from_offset_(curr_header->prev_off);
    return prev_header->str;
}

void StringList::clear() {
    memory_.clear();
    head_off_ = 0;
    tail_off_ = 0;
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

    const size_t str_len = size_t(str_end - str_begin);
    const size_t blk_len =
        sizeof(Header) + AlignOps::align_as(str_len + 1, sizeof(Header));

    if (!grow_(memory_.size() + blk_len)) {
        return false;
    }
    if (!memory_.resize(memory_.size() + blk_len)) {
        return false;
    }

    const offset_t curr_off = memory_.size() - blk_len;
    const offset_t prev_off = tail_off_;

    Header* curr_header = from_offset_(curr_off);
    curr_header->prev_off = prev_off;
    curr_header->next_off = 0;
    curr_header->blk_len = blk_len;

    if (size_ != 0) {
        Header* prev_header = from_offset_(prev_off);
        prev_header->next_off = curr_off;
    }

    memcpy(curr_header->str, str_begin, str_len); // copy string
    curr_header->str[str_len] = '\0';             // add null

    if (size_ == 0) {
        head_off_ = curr_off;
    }
    tail_off_ = curr_off;
    size_++;

    return true;
}

bool StringList::pop_back() {
    if (size_ == 0) {
        roc_panic("stringlist: list is empty");
    }

    Header* curr_header = from_offset_(tail_off_);
    const offset_t prev_off = curr_header->prev_off;

    if (!memory_.resize(memory_.size() - curr_header->blk_len)) {
        return false;
    }

    if (size_ > 1) {
        Header* prev_header = from_offset_(prev_off);
        prev_header->next_off = 0;
    }

    size_--;
    tail_off_ = prev_off;
    if (size_ == 0) {
        head_off_ = 0;
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
        const size_t str_len = size_t(str_end - str_begin);
        const size_t blk_len =
            sizeof(Header) + AlignOps::align_as(str_len + 1, sizeof(Header));

        const Header* curr_header = from_offset_(head_off_);
        const Header* back_header = from_offset_(tail_off_);

        for (;;) {
            if (curr_header->blk_len == blk_len
                && memcmp(curr_header->str, str_begin, str_len) == 0
                && curr_header->str[str_len] == '\0') {
                return curr_header->str;
            }
            if (curr_header == back_header) {
                break;
            }
            curr_header = from_offset_(curr_header->next_off);
        }
    }

    return NULL;
}

void StringList::sort(Order order) {
    if (size_ < 2) {
        return;
    }

    int (*compare)(const char* a, const char* b) =
        order == OrderLexical ? strcmp_lexical : strcmp_natural;

    for (;;) {
        // old good bubble sort
        bool swapped = false;

        offset_t curr_off = head_off_;
        Header* curr_header = from_offset_(curr_off);

        while (curr_off != tail_off_) {
            offset_t next_off = curr_header->next_off;
            Header* next_header = from_offset_(next_off);

            const int cmp = compare(curr_header->str, next_header->str);
            if (cmp > 0) {
                swap_(curr_off, curr_header, next_off, next_header);
                swapped = true;
            } else {
                curr_off = next_off;
                curr_header = next_header;
            }
        }

        if (!swapped) {
            break;
        }
    }
}

void StringList::swap_(offset_t x_off,
                       Header* x_header,
                       offset_t y_off,
                       Header* y_header) {
    offset_t prev_off = x_header->prev_off;
    Header* prev_header = from_offset_(prev_off);

    offset_t next_off = y_header->next_off;
    Header* next_header = from_offset_(next_off);

    x_header->next_off = next_off;
    x_header->prev_off = y_off;

    y_header->next_off = x_off;
    y_header->prev_off = prev_off;

    if (x_off == head_off_) {
        head_off_ = y_off;
    } else {
        prev_header->next_off = y_off;
    }

    if (y_off == tail_off_) {
        tail_off_ = x_off;
    } else {
        next_header->prev_off = x_off;
    }
}

StringList::offset_t StringList::to_offset_(const Header* header) const {
    if (!header) {
        return 0;
    }
    return offset_t((const char*)header - (const char*)memory_.data());
}

const StringList::Header* StringList::from_offset_(offset_t off) const {
    return (const Header*)(memory_.data() + off);
}

StringList::Header* StringList::from_offset_(offset_t off) {
    return (Header*)(memory_.data() + off);
}

void StringList::check_member_(const char* str) const {
    if (size_ == 0) {
        roc_panic("stringlist: list is empty");
    }

    const char* begin = &memory_[0];
    const char* end = &memory_[0] + memory_.size();

    if (str < begin || str >= end) {
        roc_panic("stringlist: string doesn't belong to the list");
    }
}

bool StringList::grow_(size_t new_size) {
    if (new_size < MinCapacity) {
        new_size = MinCapacity;
    }

    return memory_.grow_exp(new_size);
}

} // namespace core
} // namespace roc
