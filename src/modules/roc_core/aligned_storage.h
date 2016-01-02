/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/aligned_storage.h
//! @brief Aligned storage.

#ifndef ROC_CORE_ALIGNED_STORAGE_H_
#define ROC_CORE_ALIGNED_STORAGE_H_

#include "roc_core/attributes.h"
#include "roc_core/helpers.h"

namespace roc {
namespace core {

//! Aligned storage.
template <class T> class AlignedStorage {
public:
    //! Get reference to const T.
    const T& ref() const {
        return *(const T*)(const void*)mem();
    }

    //! Get reference to T.
    T& ref() {
        return *(T*)(void*)mem();
    }

    //! Get pointer to raw memory.
    const unsigned char* mem() const {
        return storage_.mem;
    }

    //! Get pointer to raw memory.
    unsigned char* mem() {
        return storage_.mem;
    }

    //! Get container.
    static AlignedStorage& container_of(T& obj) {
        return *ROC_CONTAINER_OF(ROC_CONTAINER_OF(&obj, AlignAs<T>, mem), AlignedStorage,
                                 storage_);
    }

private:
    template <class X, bool IsGe> struct AlignGeType;

    template <class X> struct AlignGeType<X, true> { typedef X type; };
    template <class X> struct AlignGeType<X, false> { typedef char type; };

    template <class X, class Y> struct AlignGe {
        typedef typename AlignGeType<Y, (sizeof(X) >= sizeof(Y))>::type type;
    };

    template <class X> union AlignAs {
        typename AlignGe<X, char>::type p_char;
        typename AlignGe<X, short>::type p_short;
        typename AlignGe<X, int>::type p_int;
        typename AlignGe<X, long>::type p_long;
        typename AlignGe<X, float>::type p_float;
        typename AlignGe<X, double>::type p_double;
        typename AlignGe<X, long double>::type p_long_double;
        typename AlignGe<X, void*>::type p_ptr;
        typename AlignGe<X, void (*)()>::type p_func_ptr;
        typename AlignGe<X, int AlignAs::*>::type p_mem_ptr;
        typename AlignGe<X, void (AlignAs::*)()>::type p_meth_ptr;
        unsigned char mem[sizeof(X)];
    };

    AlignAs<T> storage_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALIGNED_STORAGE_H_
