#pragma once

#include <stddef.h>
// Arena allocator inspired by Zig's std.heap.ArenaAllocator.
// It's like any ordinary arena allocator, but it can do some extra things:
// - free the most recent allocation (stack-based allocations)
// - grow indefinitely (up until the root allocator stops giving up bytes)

// This one in particular is optimized for a large number of small allocations - very large allocations (larger than say 100kib) should be rare.

// Differences from the Zig version:
// - This allocates the smallest number of needed fixed size blocks instead of creating buffers 1.5x the size of the requested allocation
//     - this decision was made on the assumption that the underlying allocator is optimized for a specific large allocation size (which is the case for a raw OS page alloc)

// TODO(bluesillybeard): Make some proper tests for this arena allocator so we know it works reliably and consistently

// memory block, minus two size_t's worth of bytes for some metadata. Effectively a linked list.
struct grug_arena_block {
    // the full size of this block. Will be a multiple of the block size.
    size_t total_size;
    // null for the last block
    struct grug_arena_block* next;
};

struct grug_arena {
    // The allocator will allocate in blocks of this size or a multiple of this size.
    size_t block_size;
    // Blocks are placed backwards, so the last block in the list is the oldest.
    struct grug_arena_block* blocks;
    // empty blocks, reset() moves blocks into here instead of freeing them if the caller requests some additional reserve beyond the initial capacity
    struct grug_arena_block* empty_blocks;
    // The allocator will only allocate from the last block.
    // So instead of storing the used amount for each block, we store it once for the 'surface' or 'top' block that we allocate from.
    // It does not include the space taken up by the block struct itself.
    size_t last_block_used;
};

void grug_arena_init(struct grug_arena* me, size_t initial_capacity, size_t block_size);

void* grug_arena_allocate(struct grug_arena* me, size_t size);

void* grug_arena_allocate_aligned(struct grug_arena* me, size_t size, size_t alignment);

void* grug_arena_reallocate(struct grug_arena* me, void* ptr, size_t size, size_t new_size);

void grug_arena_free(struct grug_arena* me, void* ptr, size_t size);

/// Note: keep is merely a *hint* to how much memory to keep around, not an exact quantity.
/// Generally the allocator might have an extra block compared to keep if keep doesn't perfectly align with an integer number of blocks.
void grug_arena_reset(struct grug_arena* me, size_t keep);

void grug_arena_deinit(struct grug_arena* me);
