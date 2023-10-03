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

int HeapArena::enable_leak_detection_ = false;

HeapArena::HeapArena()
    : num_allocations_(0) {
}

HeapArena::~HeapArena() {
    if (num_allocations_ != 0) {
        if (AtomicOps::load_seq_cst(enable_leak_detection_)) {
            roc_panic("heap arena: detected leak(s): %d objects was not freed",
                      (int)num_allocations_);
        }
    }
}

void HeapArena::enable_leak_detection() {
    AtomicOps::store_seq_cst(enable_leak_detection_, true);
}

size_t HeapArena::num_allocations() const {
    return (size_t)num_allocations_;
}

void* HeapArena::allocate(size_t size) {
    num_allocations_++;

    size_t total_size = (sizeof(Chunk) + sizeof(ChunkCanary) + size + sizeof(ChunkCanary)
                         + sizeof(AlignMax) - 1)
        / sizeof(AlignMax) * sizeof(AlignMax);
    size_t size_padding = AlignOps::align_max(size) - size;

    Chunk* chunk = (Chunk*)malloc(total_size);

    char* canary_before = (char*)chunk->data;
    char* memory = (char*)chunk->data + sizeof(ChunkCanary);
    char* canary_after = (char*)chunk->data + sizeof(ChunkCanary) + size;

    MemoryOps::prepare_canary(canary_before, sizeof(ChunkCanary));
    MemoryOps::poison_before_use(memory, size);
    MemoryOps::prepare_canary(canary_after, size_padding + sizeof(ChunkCanary));

    chunk->size = size;

    return memory;
}

void HeapArena::deallocate(void* ptr) {
    if (!ptr) {
        roc_panic("heap arena: null pointer");
    }

    const int n = num_allocations_--;

    if (n == 0) {
        roc_panic("heap arena: unpaired deallocate");
    }

    Chunk* chunk = ROC_CONTAINER_OF((char*)ptr - sizeof(ChunkCanary), Chunk, data);

    size_t size = chunk->size;
    size_t size_padding = AlignOps::align_max(size) - size;

    char* canary_before = (char*)chunk->data;
    char* memory = (char*)chunk->data + sizeof(ChunkCanary);
    char* canary_after = (char*)chunk->data + sizeof(ChunkCanary) + size;

    bool canary_before_ok = MemoryOps::check_canary(canary_before, sizeof(ChunkCanary));
    bool canary_after_ok =
        MemoryOps::check_canary(canary_after, size_padding + sizeof(ChunkCanary));
    if (!canary_before_ok || !canary_after_ok) {
        roc_panic("heap arena: buffer overflow detected");
    }

    MemoryOps::poison_after_use(memory, chunk->size);

    free(chunk);
}

} // namespace core
} // namespace roc
