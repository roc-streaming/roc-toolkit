/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/seqlock_impl.h
//! @brief Seqlock implementation.

#ifndef ROC_CORE_SEQLOCK_IMPL_H_
#define ROC_CORE_SEQLOCK_IMPL_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

typedef uint32_t seqlock_version_t;

//! Seqlock implementation.
class SeqlockImpl {
public:
    //! Initialize.
    SeqlockImpl();

    //! Load value version.
    seqlock_version_t version() const;

    //! Try to store value.
    bool try_store(seqlock_version_t& ver,
                   void* current_value,
                   size_t value_size,
                   const void* new_value);

    //! Store value.
    void exclusive_store(seqlock_version_t& ver,
                         void* current_value,
                         size_t value_size,
                         const void* new_value);

    //! Try to load value and version.
    bool try_load_repeat(seqlock_version_t& ver,
                         const void* current_value,
                         size_t value_size,
                         void* return_value) const;

    //! Load value and version.
    void wait_load(seqlock_version_t& ver,
                   const void* current_value,
                   size_t value_size,
                   void* return_value) const;

private:
    bool try_load_(seqlock_version_t& ver,
                   const void* current_value,
                   size_t value_size,
                   void* return_value) const;

    seqlock_version_t ver_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEQLOCK_IMPL_H_
