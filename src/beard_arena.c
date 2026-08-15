// Retrieved from https://github.com/bluesillybeard/BeardArena/blob/main/beard_arena.c on August 15 2026

// Modifications are here
#include "grug_options.h"
#define BEARD_MALLOC GRUG_MALLOC
#define BEARD_FREE GRUG_FREE

#include <stdint.h>
#include <string.h>

#include "beard_arena.h"

#ifndef BEARD_MALLOC_HEADER
    #include <malloc.h>
#endif

#ifndef BEARD_MALLOC
    #define BEARD_MALLOC(_size) malloc(_size);
#endif

#ifndef BEARD_FREE
    #define BEARD_FREE(_ptr, _size) free(_ptr);
#endif

static void beard_arena_guarantee_capacity(struct beard_arena* me, size_t cap) {
    // We assume the user is asking for a *continuous* block of x bytes
    if(cap == 0) {
        return;
    }

    if(me->blocks) {
        // See if there is already enough space on the top of the block stack
        size_t size_rem = me->blocks->total_size - me->last_block_used - sizeof(struct beard_arena_block);
        if(size_rem >= cap) {
            return;
        }
    }

    // See if there are any empty blocks with enough space
    struct beard_arena_block* prev_block = 0;
    struct beard_arena_block* block = me->empty_blocks;
    size_t iteration = 0;
    // this would be slow if there are a lot of clear blocks and none/few of them are big enough, so there is an iteration limit at which point we just give up and make a new block
    while(block && iteration<5) {
        if(block->total_size - sizeof(struct beard_arena_block) >= cap) {
            // Nice, pull it out of the free list, put it on the stack, and get out of here
            if(prev_block) {
                prev_block->next = block->next;
            } else {
                me->empty_blocks = block->next;
            }
            block->next = me->blocks;
            me->blocks = block;
            return;
        }
        prev_block = block;
        block = block->next;
        iteration+=1;
    }

    // So, we couldn't find an existing block with enough space, gotta make a new allocation
    // round up to the nearest multiple of blocks including the overhead of the block metadata
    size_t cap_with_overhead = ((cap + me->block_size + sizeof(struct beard_arena_block) - 1) / me->block_size) * me->block_size;
    struct beard_arena_block* new_block = BEARD_MALLOC(cap_with_overhead);
    new_block->total_size = cap_with_overhead;
    new_block->next = me->blocks;
    me->blocks = new_block;
}

void beard_arena_init(struct beard_arena* me, size_t initial_capacity, size_t block_size) {
    me->block_size = block_size;
    me->blocks = 0;
    me->empty_blocks = 0;
    me->last_block_used = 0;

    beard_arena_guarantee_capacity(me, initial_capacity);
}

void* beard_arena_allocate(struct beard_arena* me, size_t size) {
    // C makes finding the fundamental alignment rather difficult...
    // ... So just like pretend it's always 16 for now.
    // Unlike a "normal" allocator impl, aligned alloc does not impart any meaningful downside over regular alloc.
    // Worst case is it allocates 15 more bytes than was needed, big deal.
    return beard_arena_allocate_aligned(me, size, 16);
}

void* beard_arena_allocate_aligned(struct beard_arena* me, size_t size, size_t alignment) {
    beard_arena_guarantee_capacity(me, size);
    // guaranteeCapacity puts the space on the top of the stack so we can just yoink some out willy nilly
    char* block_start = (char*)me->blocks;
    uintptr_t first_free_spot = (uintptr_t) (block_start + sizeof(struct beard_arena_block) + me->last_block_used);
    // align the spot forward
    uintptr_t return_me = ((first_free_spot + alignment - 1) / alignment) * alignment;
    me->last_block_used += size + (return_me - first_free_spot);
    return (void*)return_me;
}

void* beard_arena_reallocate(struct beard_arena* me, void* ptr, size_t size, size_t new_size) {
    if(new_size < size) {
        return ptr;
    }
    size_t extra_space_needed = new_size - size;
    // Check if the pointer is at the top of the stack
    if(((char*)ptr) + size == ((char*)me->blocks + sizeof(struct beard_arena_block) + me->last_block_used)) {
        // Check that there is enough additional space
        if(me->blocks->total_size - sizeof(struct beard_arena_block) - me->last_block_used >= extra_space_needed) {
            me->last_block_used -= extra_space_needed;
            return ptr;
        }
    }
    // Just redo the allocation at this point
    void* new = beard_arena_allocate(me, new_size);
    memcpy(new, ptr, size);
    return new;
}

void beard_arena_free(struct beard_arena* me, void* ptr, size_t size) {
    // Check if the pointer is at the top of the stack
    if(((char*)ptr) + size == ((char*)me->blocks + sizeof(struct beard_arena_block) + me->last_block_used)) {
        // Hooray, reclaim the space
        me->last_block_used -= size;
    }
}

void beard_arena_reset(struct beard_arena* me, size_t keep) {
    // Move all of the blocks to the clear list to start
    struct beard_arena_block* block = me->blocks;
    while(block) {
        struct beard_arena_block* next = block->next;
        block->next = me->empty_blocks;
        me->empty_blocks = block;
        block = next;
    }
    me->blocks = 0;
    me->last_block_used = 0;

    // Bucket sort (theoretically) works well here since blocks are always going to be an integer number of the block size
    // And there should be a good distribution of mostly smaller blocks with a few much larger ones,
    // as most temp allocations are either super tiny temp strings or large objects like files.
    // Blocks larger than 32*blockSize will be dealt with later
    struct beard_arena_block* buckets[32] = {0};

    struct beard_arena_block* prev_block = 0;
    block = me->empty_blocks;
    while(block) {
        struct beard_arena_block* next = block->next;
        // -1 so index 0 has size of 1 * blockSize
        size_t bucket_index = (block->total_size / me->block_size) - 1;
        if(bucket_index < 32) {
            // remove the block from the list maintaining emptyBlocks as valid
            if(prev_block) {
                prev_block->next = next;
            } else {
                me->empty_blocks = next;
            }
            // add the block into the appropriate bucket
            block->next = buckets[bucket_index];
            buckets[bucket_index] = block;
        } else {
            // if block was not moved into a bucket, it's the previous block in the list
            // If block WAS moved into a bucket, prevBlock remains the same
            prev_block = block;
        }
        block = next;
    }

    // Count buckets from the largest to smallest until there is enough kept
    // After that just free them
    size_t kept = 0;
    for(size_t i = 31;; --i) {
        block = buckets[i];
        if(kept >= keep) {
            buckets[i] = 0;
        } else {
            while(block) {
                if(kept >= keep) {
                    // sever the list since everything after block will be freed
                    struct beard_arena_block* next = block->next;
                    block->next = 0;
                    block = next;
                    break;
                }
                kept += block->total_size;
                block = block->next;
            }
        }
        // block and everything after it shall be freed
        while(block) {
            struct beard_arena_block* next = block->next;
            BEARD_FREE(block, block->total_size);
            block = next;
        }
        // Iterating backwards to and including zero with an unsigned integer is super nice and not annoying at all
        if(i == 0) {
            break;
        }
    }

    // Collect all the buckets into a list so the head is the largest one
    // The ones added first end up at the bottom, and we want the smallest ones at the bottom
    // Putting the largest ones on top maximizes the chance of guaranteeCapacity finding a large empty block if the user requests a large capacity.
    struct beard_arena_block* new_list = 0;
    for(size_t i = 0; i < 32; ++i) {
        block = buckets[i];
        while(block) {
            struct beard_arena_block* next = block->next;
            block->next = new_list;
            new_list = block;
            block = next;
        }
    }

    // Now add whatever was left (no need to maintain emptyBlocks now as we're emptying it anyways)
    block = me->empty_blocks;
    while(block && kept < keep) {
        struct beard_arena_block* next = block->next;
        // except blocks larger than keep, those should be super rare anyways assuming a decently large keep
        if(block->total_size <= keep) {
            kept += block->total_size;
            block->next = new_list;
            new_list = block;
        } else {
            BEARD_FREE(block, block->total_size);
        }
        block = next;
    }

    me->empty_blocks = new_list;

    // So, we've managed to reset the allocator, select the largest blocks until we hit the keep limit, and free everything that remains,
    // all in O(n) time and O(1) memory - with the caveats being that blocks larger than keep are deleted, and blocks larger than blockSize*32 are not sorted at the end
}

void beard_arena_deinit(struct beard_arena* me) {
    // Just free everything lol
    struct beard_arena_block* block = me->blocks;
    while(block) {
        struct beard_arena_block* this_block = block;
        block = block->next;
        BEARD_FREE(this_block, this_block->total_size);
    }
    block = me->empty_blocks;
    while(block) {
        struct beard_arena_block* this_block = block;
        block = block->next;
        BEARD_FREE(this_block, this_block->total_size);
    }
    // caller owns this, just reset so deinit() can be used clear the arena
    me->blocks = 0;
    me->empty_blocks = 0;
    me->last_block_used = 0;
}
