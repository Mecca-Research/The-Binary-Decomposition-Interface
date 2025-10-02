
/**
 * @file shared_arena.c
 * @brief Implementation of shared memory arena allocator
 */

#include "shared_arena.h"
#include <stdlib.h>
#include <string.h>

// Find size class for given size
static int find_size_class(size_t size) {
    for (int i = 0; i < ARENA_NUM_SIZE_CLASSES; i++) {
        if (size <= ARENA_SIZE_CLASSES[i]) {
            return i;
        }
    }
    return -1;  // Too large for size classes
}

shared_arena_t* shared_arena_create(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Allocate arena structure
    shared_arena_t* arena = calloc(1, sizeof(shared_arena_t));
    if (!arena) {
        return NULL;
    }
    
    // Allocate backing memory (page-aligned)
    arena->base_address = aligned_alloc(4096, size);
    if (!arena->base_address) {
        free(arena);
        return NULL;
    }
    
    arena->total_size = size;
    arena->used_size = 0;
    
    // Allocate bitmap (1 bit per 64 bytes)
    arena->bitmap_size = (size / 64 + 63) / 64;
    arena->allocation_bitmap = calloc(arena->bitmap_size, sizeof(uint64_t));
    if (!arena->allocation_bitmap) {
        free(arena->base_address);
        free(arena);
        return NULL;
    }
    
    // Initialize large block free list with entire arena
    free_block_t* initial_block = (free_block_t*)arena->base_address;
    initial_block->size = size;
    initial_block->next = NULL;
    arena->large_blocks = initial_block;
    
    return arena;
}

void shared_arena_destroy(shared_arena_t* arena) {
    if (arena) {
        if (arena->allocation_bitmap) {
            free(arena->allocation_bitmap);
        }
        if (arena->base_address) {
            free(arena->base_address);
        }
        free(arena);
    }
}

void* shared_arena_alloc(shared_arena_t* arena, size_t size) {
    if (!arena || size == 0) {
        return NULL;
    }
    
    // Add header size
    size_t total_size = size + sizeof(size_t);
    
    // Find size class
    int size_class = find_size_class(total_size);
    
    void* ptr = NULL;
    
    if (size_class >= 0) {
        // Use size class free list
        size_t class_size = ARENA_SIZE_CLASSES[size_class];
        
        if (arena->free_lists[size_class]) {
            // Reuse from free list
            free_block_t* block = arena->free_lists[size_class];
            arena->free_lists[size_class] = block->next;
            ptr = block;
        } else {
            // Allocate from large blocks
            free_block_t** prev_ptr = &arena->large_blocks;
            free_block_t* block = arena->large_blocks;
            
            while (block) {
                if (block->size >= class_size) {
                    // Split block if large enough
                    if (block->size >= class_size + sizeof(free_block_t) + 64) {
                        free_block_t* remainder = (free_block_t*)((char*)block + class_size);
                        remainder->size = block->size - class_size;
                        remainder->next = block->next;
                        *prev_ptr = remainder;
                    } else {
                        // Use entire block
                        *prev_ptr = block->next;
                    }
                    
                    ptr = block;
                    break;
                }
                prev_ptr = &block->next;
                block = block->next;
            }
        }
        
        if (ptr) {
            // Store size in header
            *(size_t*)ptr = size;
            ptr = (char*)ptr + sizeof(size_t);
            
            arena->used_size += class_size;
            arena->stats.total_allocations++;
            arena->stats.current_allocated += size;
            if (arena->stats.current_allocated > arena->stats.peak_allocated) {
                arena->stats.peak_allocated = arena->stats.current_allocated;
            }
        }
    } else {
        // Large allocation - use large blocks directly
        free_block_t** prev_ptr = &arena->large_blocks;
        free_block_t* block = arena->large_blocks;
        
        while (block) {
            if (block->size >= total_size) {
                // Split block if large enough
                if (block->size >= total_size + sizeof(free_block_t) + 64) {
                    free_block_t* remainder = (free_block_t*)((char*)block + total_size);
                    remainder->size = block->size - total_size;
                    remainder->next = block->next;
                    *prev_ptr = remainder;
                } else {
                    *prev_ptr = block->next;
                }
                
                ptr = block;
                
                // Store size in header
                *(size_t*)ptr = size;
                ptr = (char*)ptr + sizeof(size_t);
                
                arena->used_size += total_size;
                arena->stats.total_allocations++;
                arena->stats.current_allocated += size;
                if (arena->stats.current_allocated > arena->stats.peak_allocated) {
                    arena->stats.peak_allocated = arena->stats.current_allocated;
                }
                
                break;
            }
            prev_ptr = &block->next;
            block = block->next;
        }
    }
    
    return ptr;
}

void* shared_arena_alloc_dma(shared_arena_t* arena, size_t size) {
    if (!arena || size == 0) {
        return NULL;
    }
    
    // For DMA, we need 4KB alignment
    // This is a simplified implementation - just allocate from large blocks
    size_t total_size = size + sizeof(size_t);
    size_t aligned_size = (total_size + ARENA_DMA_ALIGNMENT - 1) & ~(ARENA_DMA_ALIGNMENT - 1);
    
    free_block_t** prev_ptr = &arena->large_blocks;
    free_block_t* block = arena->large_blocks;
    
    while (block) {
        // Check if block is aligned and large enough
        uintptr_t addr = (uintptr_t)block;
        if (addr % ARENA_DMA_ALIGNMENT == 0 && block->size >= aligned_size) {
            // Split block if large enough
            if (block->size >= aligned_size + sizeof(free_block_t) + ARENA_DMA_ALIGNMENT) {
                free_block_t* remainder = (free_block_t*)((char*)block + aligned_size);
                remainder->size = block->size - aligned_size;
                remainder->next = block->next;
                *prev_ptr = remainder;
            } else {
                *prev_ptr = block->next;
            }
            
            // Store size in header
            *(size_t*)block = size;
            void* ptr = (char*)block + sizeof(size_t);
            
            arena->used_size += aligned_size;
            arena->stats.total_allocations++;
            arena->stats.current_allocated += size;
            if (arena->stats.current_allocated > arena->stats.peak_allocated) {
                arena->stats.peak_allocated = arena->stats.current_allocated;
            }
            
            return ptr;
        }
        prev_ptr = &block->next;
        block = block->next;
    }
    
    return NULL;  // No suitable aligned block found
}

void shared_arena_free(shared_arena_t* arena, void* ptr, size_t size) {
    if (!arena || !ptr) {
        return;
    }
    
    // Get actual pointer (before header)
    void* actual_ptr = (char*)ptr - sizeof(size_t);
    size_t stored_size = *(size_t*)actual_ptr;
    
    // Verify size matches
    if (stored_size != size) {
        // Size mismatch - potential corruption
        return;
    }
    
    size_t total_size = size + sizeof(size_t);
    
    // Find size class
    int size_class = find_size_class(total_size);
    
    if (size_class >= 0) {
        // Return to size class free list
        free_block_t* block = (free_block_t*)actual_ptr;
        block->next = arena->free_lists[size_class];
        arena->free_lists[size_class] = block;
        
        arena->used_size -= ARENA_SIZE_CLASSES[size_class];
    } else {
        // Return to large blocks
        free_block_t* block = (free_block_t*)actual_ptr;
        block->size = total_size;
        block->next = arena->large_blocks;
        arena->large_blocks = block;
        
        arena->used_size -= total_size;
    }
    
    arena->stats.total_frees++;
    arena->stats.current_allocated -= size;
}

void shared_arena_get_stats(const shared_arena_t* arena, arena_stats_t* stats) {
    if (arena && stats) {
        *stats = arena->stats;
    }
}

void shared_arena_reset_stats(shared_arena_t* arena) {
    if (arena) {
        memset(&arena->stats, 0, sizeof(arena_stats_t));
    }
}
