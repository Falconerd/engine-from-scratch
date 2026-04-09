#pragma once

#include "../types.h"

// Fixed-size memory arena
typedef struct Arena {
	void *data;
	usize capacity;
	usize used;
} Arena;

// NOTE: 16 bytes on 64-bit machines
#define ARENA_DEFAULT_ALIGNMENT (2 * sizeof(void *))

Arena arena_create(void *buffer, usize capacity);
void *arena_alloc_internal(Arena *arena, usize size, usize alignment);
void *arena_alloc(Arena *arena, usize size);
void *arena_alloc_aligned(Arena *arena, usize size, usize alignment);
void arena_clear(Arena *arena);

#ifdef _MSC_VER
	#define ALIGN_OF(T) __alignof(T)
#else
	#define ALIGN_OF(T) __alignof__(T)
#endif

#define ARENA_PUSH(a, T) ((T*)arena_alloc_aligned(a, sizeof(T), ALIGN_OF(T)))
#define ARENA_PUSH_N(a, T, n) ((T*)arena_alloc_aligned(a, sizeof(T) * (n), ALIGN_OF(T)))

#define ARENA_LEN(a, T) (a.used / sizeof(T))

