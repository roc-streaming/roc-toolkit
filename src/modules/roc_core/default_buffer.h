/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/default_buffer.h
//! @brief Default IBuffer implementation.

#ifndef ROC_CORE_DEFAULT_BUFFER_H_
#define ROC_CORE_DEFAULT_BUFFER_H_

#include "roc_core/helpers.h"
#include "roc_core/ibuffer.h"
#include "roc_core/ipool.h"
#include "roc_core/panic.h"
#include "roc_core/print_buffer.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Default IBuffer implementation.
template <size_t MaxSz, class T> class DefaultBuffer : public IBuffer<T> {
public:
    //! Initialize empty buffer.
    explicit DefaultBuffer(IPool<DefaultBuffer>& pool)
        : magic_(MAGIC_)
        , size_(0)
        , pool_(pool) {
        //
        memset(&storage_, GUARD_, sizeof(storage_));
        check_guards_();
    }

    ~DefaultBuffer() {
        check_guards_();
    }

    //! Get buffer data.
    virtual T* data() {
        return storage_.data;
    }

    //! Get buffer data.
    virtual const T* data() const {
        check_guards_();
        return storage_.data;
    }

    //! Get maximum allowed number of elements.
    virtual size_t max_size() const {
        return MaxSz;
    }

    //! Get number of elements.
    virtual size_t size() const {
        return size_;
    }

    //! Set number of elements in buffer.
    //! @remarks
    //!  @p sz should be no more than max_size().
    virtual void set_size(size_t sz) {
        if (sz > MaxSz) {
            roc_panic("attempting to set too large buffer size (%lu > %lu)",
                      (unsigned long)sz, (unsigned long)MaxSz);
        }
        size_ = sz;
        *size_guard_() = GUARD_;
    }

    //! Check buffer integrity.
    virtual void check() const {
        check_guards_();
    }

    //! Print buffer to stdout.
    virtual void print() const {
        print_buffer(data(), size(), max_size());
    }

    //! Restore existing buffer by pointer to its data.
    static DefaultBuffer* container_of(T* data_ptr) {
        if (data_ptr == NULL) {
            roc_panic("attempting to pass null to container_of");
        }

        DefaultBuffer* buff = ROC_CONTAINER_OF(ROC_CONTAINER_OF(data_ptr, Storage, data),
                                               DefaultBuffer, storage_);

        buff->check_guards_();
        return buff;
    }

private:
    virtual void free() {
        pool_.destroy(*this);
    }

private:
    static const size_t MAGIC_ = 0xdeadbeaf;
    static const uint8_t GUARD_ = 0xcc;

    uint8_t* head_guard_() {
        return ((uint8_t*)&storage_.head_guard) + offsetof(Storage, data) - 1;
    }

    uint8_t* tail_guard_() {
        return (uint8_t*)(storage_.data + MaxSz);
    }

    uint8_t* size_guard_() {
        return (uint8_t*)(storage_.data + size_);
    }

    void setup_guards_() {
        *head_guard_() = GUARD_;
        *tail_guard_() = GUARD_;
        *size_guard_() = GUARD_;
    }

    void check_guards_() {
        if (magic_ != MAGIC_) {
            roc_panic("buffer contains invalid magic (corrupted pointer?)");
        }
        if (*head_guard_() != GUARD_) {
            roc_panic("buffer overflow detected (head guard, size = %lu)",
                      (unsigned long)size_);
        }
        if (*tail_guard_() != GUARD_) {
            roc_panic("buffer overflow detected (tail guard, size = %lu)",
                      (unsigned long)size_);
        }
        if (*size_guard_() != GUARD_) {
            roc_panic("buffer overflow detected (size guard, size = %lu)",
                      (unsigned long)size_);
        }
    }

    void check_guards_() const {
        // check_guards_() doesn't modify the object.
        const_cast<DefaultBuffer*>(this)->check_guards_();
    }

private:
    struct Storage {
        uint64_t head_guard; // uint64_t forces maximum alignment for data
        T data[MaxSz];
        uint8_t tail_guard;
    };

    size_t magic_;
    size_t size_;

    Storage storage_;

    IPool<DefaultBuffer>& pool_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_DEFAULT_BUFFER_H_
