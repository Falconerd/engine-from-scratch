#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../types.h"
#include "../util.h"
#include "../io.h"

// 20 MiB, can probably change this to a higher value without issue.
// Check your target platform.
#define IO_READ_CHUNK_SIZE 2097152
#define IO_READ_ERROR_GENERAL "Error reading filie: %s. errno: %d\n"
#define IO_READ_ERROR_MEMORY "Not enough free memory to read file: %s\n"

// Adapted from https://stackoverflow.com/a/44894946 (not the chosen answer) by Nominal Animal
File io_file_read(const char *path) {
	File file = { .is_valid = false };

	FILE *fp = fopen(path, "rb");
	if (!fp || ferror(fp)) {
		ERROR_RETURN(file, IO_READ_ERROR_GENERAL, path, errno);
	}

	char *data = NULL;
	char *tmp;
	usize used = 0;
	usize size = 0;
	usize n;

	while (true) {
		if (used + IO_READ_CHUNK_SIZE + 1 > size) {
			size = used + IO_READ_CHUNK_SIZE + 1;

			if (size <= used) {
				free(data);
				ERROR_RETURN(file, "Input file too large: %s\n", path);
			}

			tmp = realloc(data, size);
			if (!tmp) {
				free(data);
				ERROR_RETURN(file, IO_READ_ERROR_MEMORY, path);
			}
			data = tmp;
		}

		n = fread(data + used, 1, IO_READ_CHUNK_SIZE, fp);
		if (n == 0)
			break;

		used += n;
	}

	if (ferror(fp)) {
		free(data);
		ERROR_RETURN(file, IO_READ_ERROR_GENERAL, path, errno);
	}

	tmp = realloc(data, used + 1);
	if (!tmp) {
		free(data);
		ERROR_RETURN(file, IO_READ_ERROR_MEMORY, path);
	}
	data = tmp;
	data[used] = 0;

	file.data = data;
	file.len = used;
	file.is_valid = true;

	return file;
}

int io_file_write(void *buffer, usize size, const char *path) {
	FILE *fp = fopen(path, "wb");
	if (!fp || ferror(fp))
		ERROR_RETURN(1, "Cannot write file: %s.\n", path);

	usize chunks_written = fwrite(buffer, size, 1, fp);

	fclose(fp);

	if (chunks_written != 1)
		ERROR_RETURN(1, "Write error. "
				"Expected 1 chunk, got %zu.\n", chunks_written);

	return 0;
}

