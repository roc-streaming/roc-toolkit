/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/heap_arena.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/memory_ops.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

size_t HeapArena::flags_ = DefaultHeapArenaFlags;

HeapArena::HeapArena()
    : num_allocations_(0)
    , num_guard_failures_(0) {
}

HeapArena::~HeapArena() {
    if (num_allocations_ != 0) {
        if (AtomicOps::load_seq_cst(flags_) & HeapArenaFlag_EnableLeakDetection) {
            roc_panic("heap arena: detected leak(s): %d objects was not freed",
                      (int)num_allocations_);
        }
    }
}

void HeapArena::set_flags(size_t flags) {
    AtomicOps::store_seq_cst(flags_, flags);
}

size_t HeapArena::num_allocations() const {
    return (size_t)num_allocations_;
}

size_t HeapArena::num_guard_failures() const {
    return num_guard_failures_;
}

void* HeapArena::allocate(size_t size) {
    num_allocations_++;

    size_t chunk_size =
        sizeof(ChunkHeader) + sizeof(ChunkCanary) + size + sizeof(ChunkCanary);

    ChunkHeader* chunk = (ChunkHeader*)malloc(chunk_size);

    chunk->owner = this;

    char* canary_before = (char*)chunk->data;
    char* memory = (char*)chunk->data + sizeof(ChunkCanary);
    char* canary_after = (char*)chunk->data + sizeof(ChunkCanary) + size;

    MemoryOps::prepare_canary(canary_before, sizeof(ChunkCanary));
    MemoryOps::poison_before_use(memory, size);
    MemoryOps::prepare_canary(canary_after, sizeof(ChunkCanary));

    chunk->size = size;

    return memory;
}

void HeapArena::deallocate(void* ptr) {
    if (!ptr) {
        roc_panic("heap arena: null pointer");
    }

    ChunkHeader* chunk =
        ROC_CONTAINER_OF((char*)ptr - sizeof(ChunkCanary), ChunkHeader, data);

    size_t size = chunk->size;

    char* canary_before = (char*)chunk->data;
    char* memory = (char*)chunk->data + sizeof(ChunkCanary);
    char* canary_after = (char*)chunk->data + sizeof(ChunkCanary) + size;

    const bool canary_before_ok =
        MemoryOps::check_canary(canary_before, sizeof(ChunkCanary));
    const bool canary_after_ok =
        MemoryOps::check_canary(canary_after, sizeof(ChunkCanary));

    if (!canary_before_ok || !canary_after_ok) {
        num_guard_failures_++;
        if (AtomicOps::load_seq_cst(flags_) & HeapArenaFlag_EnableGuards) {
            roc_panic("heap arena: detected memory violation: ok_before=%d ok_after=%d",
                      (int)canary_before_ok, (int)canary_after_ok);
        }
    }

    const bool is_owner = chunk->owner == this;

    if (!is_owner) {
        num_guard_failures_++;
        if (AtomicOps::load_seq_cst(flags_) & HeapArenaFlag_EnableGuards) {
            roc_panic("heap arena: attempt to deallocate chunk not belonging to this "
                      "heap arena:"
                      " this_pool=%p chunk_pool=%p",
                      (const void*)this, (const void*)chunk->owner);
        }
        return;
    }

    const int n = num_allocations_--;

    if (n == 0) {
        roc_panic("heap arena: unpaired deallocate");
    }

    MemoryOps::poison_after_use(memory, chunk->size);

    free(chunk);
}

} // namespace core
} // namespace roc
