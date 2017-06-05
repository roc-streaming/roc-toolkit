/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer_traits.h
//! @brief Buffer traits.

#ifndef ROC_CORE_BUFFER_TRAITS_H_
#define ROC_CORE_BUFFER_TRAITS_H_

#include "roc_core/buffer_slice.h"
#include "roc_core/default_buffer.h"
#include "roc_core/default_buffer_composer.h"
#include "roc_core/ibuffer.h"
#include "roc_core/ibuffer_composer.h"
#include "roc_core/ipool.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/singleton.h"

namespace roc {
namespace core {

//! Buffer traits.
template <class T> struct BufferTraits {
    //! IBuffer interface.
    typedef IBuffer<T> Interface;

    //! IBuffer shared pointer.
    typedef SharedPtr<Interface> Ptr;

    //! IBuffer const shared pointer.
    typedef SharedPtr<Interface const> ConstPtr;

    //! IBuffer slice.
    typedef BufferSlice<T, Interface> Slice;

    //! IBuffer const slice.
    typedef BufferSlice<T const, Interface const> ConstSlice;

    //! IBufferComposer interface.
    typedef IBufferComposer<T> Composer;

    //! Default composer type.
    template <size_t BufSz> static Composer& default_composer() {
        typedef DefaultBufferComposer<BufSz, T> //
            Composer;

        return Singleton<Composer>::instance();
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_TRAITS_H_
