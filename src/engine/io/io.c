#include <stdio.h>
#include "io.h"
#include "../util.h"

char *io_file_read(const char *path) {
	FILE *fp = fopen(path, "r");

	if (!fp) {
		ERROR_RETURN(NULL, "Cannot read file %s\n", path);
	}

	fseek(fp, 0, SEEK_END);

	i32 length = ftell(fp);

	if (length == -1L) {
		ERROR_RETURN(NULL, "Could not assertain length of file %s\n", path);
	}

	fseek(fp, 0, SEEK_SET);

	char *buffer = malloc((length + 1) * sizeof(char));
	if (!buffer) {
		ERROR_RETURN(NULL, "Cannot allocate file buffer for %s\n", path);
	}

	fread(buffer, sizeof(char), length, fp);
	buffer[length] = 0;

	fclose(fp);

	printf("File loaded. %s\n", path);
	return buffer;
}

int io_file_write(void *buffer, size_t size, const char *path) {
	FILE *fp = fopen(path, "w");
	if (!fp) {
		ERROR_RETURN(1, "Cannot write file %s\n", path);
	}

	size_t chunks_written = fwrite(buffer, size, 1, fp);

	fclose(fp);

	if (chunks_written != 1) {
		ERROR_RETURN(1, "Incorrect chunks written. Expected 1, got %zu.\n", chunks_written);
	}

	printf("File saved. %s\n", path);
	return 0;
}

