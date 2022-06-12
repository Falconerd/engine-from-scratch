#include <stdio.h>
#include <stdlib.h>
#include "../types.h"
#include "../util.h"
#include "io.h"

File io_file_read(const char *path) {
    File file = { .is_valid = false };

    FILE *fp = fopen(path, "r");
    if (!fp) {
        ERROR_RETURN(file, "Cannot read file %s\n", path);
    }

    fseek(fp, 0, SEEK_END);

    file.len = ftell(fp);
    if (file.len == -1L) {
        ERROR_RETURN(file, "Could not assertain length of file %s\n", path);
    }

    fseek(fp, 0, SEEK_SET);

    file.data = malloc((file.len + 1) * sizeof(char));
    if (!file.data) {
        ERROR_RETURN(file, "Cannot allocate file buffer for %s\n", path);
    }

    fread(file.data, sizeof(char), file.len, fp);
    file.data[file.len] = 0;

    fclose(fp);

    return file;
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

