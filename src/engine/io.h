#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include "types.h"

typedef struct file {
	char *data;
	usize len;
	bool is_valid;
} File;

File io_file_read(const char *path);
int io_file_write(void *buffer, usize size, const char *path);

