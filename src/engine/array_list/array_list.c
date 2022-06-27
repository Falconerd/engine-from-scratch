#include "../array_list.h"
#include "../util.h"
#include <stdlib.h>
#include <string.h>

Array_List *array_list_create(usize item_size, usize initial_capacity) {
    Array_List *list = calloc(1, sizeof(Array_List));

    if (!list)
	    ERROR_RETURN(NULL, "Could not allocate memory for array list\n");

    list->item_size = item_size;
    list->capacity = initial_capacity;
    list->len = 0;
    list->items = calloc(initial_capacity, item_size);

    if (!list->items)
	    ERROR_RETURN(NULL, "Could not allocate memory for array list items\n");

    return list;
}

usize array_list_append(Array_List *list, void *item) {
	if (list->len == list->capacity) {
		list->capacity = list->capacity > 0 ? list->capacity * 2 : 1;
		void *items = realloc(list->items, list->item_size * list->capacity);

		if (!items)
			ERROR_RETURN(-1, "Could not reallocate memory for array list items\n");

		list->items = items;
	}

	usize index = list->len++;

	memcpy((u8*)list->items + index * list->item_size, item, list->item_size);

	return index;
}

void *array_list_get(Array_List *list, usize index) {
	if (index >= list->len)
		ERROR_RETURN(NULL, "Index out of bounds\n");

	return (u8*)list->items + index * list->item_size;
}

void array_list_remove(Array_List *list, usize index) {
	if (list->len == 0)
		ERROR_RETURN(, "List is empty\n");
	if (index >= list->len)
		ERROR_RETURN(, "Index out of bounds\n");

	if (list->len == 1) {
		list->len = 0;
		return;
	}

	--list->len;

	u8 *item_ptr = (u8*)list->items + (index * list->item_size);
	u8 *end_ptr = (u8*)list->items + (list->len * list->item_size);
	memcpy(item_ptr, end_ptr, list->item_size);
}

