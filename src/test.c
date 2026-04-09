#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "engine/mem/arena.c"

#define TEST_LABEL_WIDTH 50

int main(void) {
	printf("---- ARENA ----\n");

	usize capacity = 1000;
	void *buffer = malloc(capacity);
	Arena arena = arena_create(buffer, capacity);

	printf("%-*s ... ", TEST_LABEL_WIDTH, "Create"); {
		assert(arena.data != NULL);
		assert(arena.capacity == capacity);
		assert(arena.used == 0);
	}; printf("\e[0;32mOK\e[0;0m\n");

	printf("%-*s ... ", TEST_LABEL_WIDTH, "Allocate"); {
		void *data = arena.data;
		usize capacity = arena.capacity;

		void *p = arena_alloc(&arena, 40);
		assert(arena.used == 40);
		assert(p != NULL);
		assert(p == arena.data);
		// NOTE: 64-bit only
		// 40 % 16 = 8, diff = 8, aligned offset = 48
		// 48 + 40 = 88
		void *p2 = arena_alloc(&arena, 40);
		assert(arena.used == 88);
		assert(p2 != NULL);
		assert(p2 != p);

		assert(arena.data == data);
		assert(arena.capacity == capacity);
	}; printf("\e[0;32mOK\e[0;0m\n");

	printf("%-*s ... ", TEST_LABEL_WIDTH, "Clear"); {
		void *data = arena.data;
		usize capacity = arena.capacity;

		arena_clear(&arena);
		assert(arena.used == 0);

		assert(arena.data == data);
		assert(arena.capacity == capacity);
	}; printf("\e[0;32mOK\e[0;0m\n");

	printf("%-*s ... ", TEST_LABEL_WIDTH, "Allocate aligned"); {
		arena_clear(&arena);

		// Alloc 1 byte to misalign the cursor
		arena_alloc(&arena, 1);
		assert(arena.used == 1);

		// Align to 8: cursor is at offset 1, remainder = 1, diff = 7
		// Aligned offset = 8, used = 8 + 1 = 9
		void *p = arena_alloc_aligned(&arena, 1, 8);
		assert(p != NULL);
		assert(arena.used == 9);
		assert((usize)p % 8 == 0);
	}; printf("\e[0;32mOK\e[0;0m\n");

	printf("%-*s ... ", TEST_LABEL_WIDTH, "Out of memory"); {
		Arena full_arena = arena_create(buffer, 16);
		arena_alloc(&full_arena, 16);
		void *p = arena_alloc(&full_arena, 1);
		assert(p == NULL);
	}; printf("\e[0;32mOK\e[0;0m\n");
}
