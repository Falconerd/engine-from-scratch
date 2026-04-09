#include "./arena.h"
#include "../util.h"

Arena arena_create(void *buffer, usize capacity) {
    if (buffer == NULL) ERROR_EXIT("Arena buffer must not be NULL\n");
    if (capacity == 0) ERROR_EXIT("Arena capacity must be > 0\n");
    return (Arena){
        .capacity = capacity,
        .data = buffer,
    };
}

void *arena_alloc_internal(Arena *arena, usize size, usize alignment) {
	if (size == 0) ERROR_EXIT("Arena size must be > 0\n");
	if (alignment == 0) ERROR_EXIT("Arena alignment must be > 0\n");
	void *result = NULL;

	// Get absolute address of the next free byte
	usize p = (usize)(arena->data) + arena->used;

	// How far past the aligned boundary we are (0 if aligned)
	usize remainder = p % alignment;

	// Bytes needed to reach the next boundary
	usize diff = alignment - remainder;
	if (remainder == 0) {
		diff = 0;
	}

	// Move to aligned boundary
	p += diff;

	// Save pointer for return (if bounds check succeeds)
	void *addr = (void *)p;

	// Convert back to relative offset for bounds check
	p -= (usize)(arena->data);

	if (p + size <= arena->capacity) {
		arena->used = p + size;
		result = addr;
	}

	return result;
}

void *arena_alloc(Arena *arena, usize size) {
	return arena_alloc_internal(arena, size, ARENA_DEFAULT_ALIGNMENT);
}

void *arena_alloc_aligned(Arena *arena, usize size, usize alignment) {
	return arena_alloc_internal(arena, size, alignment);
}

void arena_clear(Arena *arena) {
	arena->used = 0;
}
