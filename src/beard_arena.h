#pragma once

// Retrieved from https://github.com/bluesillybeard/BeardArena/blob/main/beard_arena.h on August 15 2026

#include <stddef.h>

// memory block, minus two size_t's worth of bytes for some metadata. Effectively a linked list.
struct beard_arena_block {
    // the full size of this block. Will be a multiple of the block size.
    size_t total_size;
    // null for the last block
    struct beard_arena_block* next;
};

struct beard_arena {
    // The allocator will allocate in blocks of this size or a multiple of this size.
    size_t block_size;
    // Blocks are placed backwards, so the last block in the list is the oldest.
    struct beard_arena_block* blocks;
    // empty blocks, reset() moves blocks into here instead of freeing them if the caller requests some additional reserve beyond the initial capacity
    struct beard_arena_block* empty_blocks;
    // The allocator will only allocate from the last block.
    // So instead of storing the used amount for each block, we store it once for the 'surface' or 'top' block that we allocate from.
    // It does not include the space taken up by the block struct itself.
    size_t last_block_used;
};

void beard_arena_init(struct beard_arena* me, size_t initial_capacity, size_t block_size);

void* beard_arena_allocate(struct beard_arena* me, size_t size);

void* beard_arena_allocate_aligned(struct beard_arena* me, size_t size, size_t alignment);

void* beard_arena_reallocate(struct beard_arena* me, void* ptr, size_t size, size_t new_size);

void beard_arena_free(struct beard_arena* me, void* ptr, size_t size);

/// Note: keep is merely a *hint* to how much memory to keep around, not an exact quantity.
/// Generally the allocator might have an extra block compared to keep if keep doesn't perfectly align with an integer number of blocks.
void beard_arena_reset(struct beard_arena* me, size_t keep);

void beard_arena_deinit(struct beard_arena* me);
