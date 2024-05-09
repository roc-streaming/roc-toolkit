/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/heap_arena.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/memory_ops.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

size_t HeapArena::guards_ = HeapArena_DefaultGuards;

HeapArena::HeapArena()
    : num_allocations_(0)
    , num_guard_failures_(0) {
}

HeapArena::~HeapArena() {
    if (num_allocations_ != 0) {
        if (report_guard_(HeapArena_LeakGuard)) {
            roc_panic("heap arena: detected leak(s): %d chunk(s) were not freed",
                      (int)num_allocations_);
        }
    }
}

void HeapArena::set_guards(size_t guards) {
    AtomicOps::store_seq_cst(guards_, guards);
}

size_t HeapArena::num_allocations() const {
    return (size_t)num_allocations_;
}

size_t HeapArena::num_guard_failures() const {
    return num_guard_failures_;
}

void* HeapArena::allocate(size_t size) {
    const size_t chunk_size =
        sizeof(ChunkHeader) + sizeof(ChunkCanary) + size + sizeof(ChunkCanary);

    ChunkHeader* chunk = (ChunkHeader*)malloc(chunk_size);

    if (!chunk) {
        roc_log(LogError,
                "heap arena: allocation failed: chunk_size=%lu payload_size=%lu",
                (unsigned long)chunk_size, (unsigned long)size);
        return NULL;
    }

    chunk->owner = this;

    char* canary_before = (char*)chunk->data;
    char* memory = (char*)chunk->data + sizeof(ChunkCanary);
    char* canary_after = (char*)chunk->data + sizeof(ChunkCanary) + size;

    MemoryOps::prepare_canary(canary_before, sizeof(ChunkCanary));
    MemoryOps::poison_before_use(memory, size);
    MemoryOps::prepare_canary(canary_after, sizeof(ChunkCanary));

    chunk->size = size;

    num_allocations_++;

    return memory;
}

void HeapArena::deallocate(void* ptr) {
    if (!ptr) {
        roc_panic("heap arena: null pointer");
    }

    ChunkHeader* chunk =
        ROC_CONTAINER_OF((char*)ptr - sizeof(ChunkCanary), ChunkHeader, data);

    const bool is_owner = chunk->owner == this;

    if (!is_owner) {
        if (report_guard_(HeapArena_OwnershipGuard)) {
            roc_panic("heap arena:"
                      " attempt to deallocate chunk not belonging to this arena:"
                      " this_arena=%p chunk_arena=%p",
                      (const void*)this, (const void*)chunk->owner);
        }
        return;
    }

    size_t size = chunk->size;

    char* canary_before = (char*)chunk->data;
    char* memory = (char*)chunk->data + sizeof(ChunkCanary);
    char* canary_after = (char*)chunk->data + sizeof(ChunkCanary) + size;

    const bool canary_before_ok =
        MemoryOps::check_canary(canary_before, sizeof(ChunkCanary));
    const bool canary_after_ok =
        MemoryOps::check_canary(canary_after, sizeof(ChunkCanary));

    if (!canary_before_ok || !canary_after_ok) {
        if (report_guard_(HeapArena_OverflowGuard)) {
            roc_panic("heap arena: detected memory violation:"
                      " header_guard=%s footer_guard=%s",
                      canary_before_ok ? "ok" : "corrupted",
                      canary_after_ok ? "ok" : "corrupted");
        }
    }

    const int n = num_allocations_--;
    if (n == 0) {
        roc_panic("heap arena: unpaired deallocate");
    }

    MemoryOps::poison_after_use(memory, chunk->size);

    free(chunk);
}

size_t HeapArena::compute_allocated_size(size_t size) const {
    return sizeof(ChunkHeader) + sizeof(ChunkCanary) + size + sizeof(ChunkCanary);
}

size_t HeapArena::allocated_size(void* ptr) const {
    if (!ptr) {
        roc_panic("heap arena: null pointer");
    }

    const ChunkHeader* chunk =
        ROC_CONTAINER_OF((char*)ptr - sizeof(ChunkCanary), ChunkHeader, data);

    const bool is_owner = chunk->owner == this;

    if (!is_owner) {
        if (report_guard_(HeapArena_OwnershipGuard)) {
            roc_panic("heap arena: attempt to get allocated size of chunk not belonging "
                      "to this arena: this_arena=%p chunk_arena=%p",
                      (const void*)this, (const void*)chunk->owner);
        }
        return 0;
    }

    return compute_allocated_size(chunk->size);
}

bool HeapArena::report_guard_(HeapArenaGuard guard) const {
    num_guard_failures_++;
    return (AtomicOps::load_seq_cst(guards_) & (size_t)guard) != 0;
}

} // namespace core
} // namespace roc
