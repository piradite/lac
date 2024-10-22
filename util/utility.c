#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"

void handle_error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        handle_error("Could not open file");
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *content = malloc(length + 1);
    if (!content) {
        handle_error("Memory allocation failed");
    }
    fread(content, 1, length, file);
    fclose(file);
    content[length] = '\0';
    return content;
}
