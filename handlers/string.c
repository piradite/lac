#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/bytecode.h"

void handle_string(const char *args, FILE *output) {
    fwrite(&(unsigned char) { STRING_OP }, 1, 1, output);

    char name[32];
    char value[256];
    char *colon_ptr = strchr(args, ':');
    char *arrow_ptr = strstr(args, "->");

    if (!colon_ptr || !arrow_ptr) {
	fprintf(stderr, "Error: Invalid string assignment syntax\n");
	exit(ERR_MALFORMED_PRINT);
    }

    size_t name_len = colon_ptr - args;
    if (name_len >= sizeof(name)) {
	fprintf(stderr, "Error: Variable name is too long\n");
	exit(ERR_MALFORMED_PRINT);
    }
    strncpy(name, args, name_len);
    name[name_len] = '\0';

    const char *value_start = strchr(arrow_ptr, '"');
    const char *value_end = strrchr(arrow_ptr, '"');
    if (!value_start || !value_end || value_end <= value_start) {
	fprintf(stderr, "Error: Invalid string value in assignment\n");
	exit(ERR_MALFORMED_PRINT);
    }

    size_t value_len = value_end - value_start - 1;
    if (value_len >= sizeof(value)) {
	fprintf(stderr, "Error: String value is too long\n");
	exit(ERR_MALFORMED_PRINT);
    }
    strncpy(value, value_start + 1, value_len);
    value[value_len] = '\0';

    fwrite(&name_len, sizeof(size_t), 1, output);
    fwrite(name, sizeof(char), name_len, output);
    fwrite(&value_len, sizeof(size_t), 1, output);
    fwrite(value, sizeof(char), value_len, output);

    set_variable(name, TYPE_STRING, value);
}

void handle_string_opcode(FILE *input) {
    size_t name_len;
    fread(&name_len, sizeof(size_t), 1, input);

    char name[32];
    fread(name, sizeof(char), name_len, input);
    name[name_len] = '\0';

    size_t value_len;
    fread(&value_len, sizeof(size_t), 1, input);
    char *value = malloc(value_len + 1);
    if (!value) {
	perror("Memory allocation failed");
	exit(1);
    }
    fread(value, sizeof(char), value_len, input);
    value[value_len] = '\0';

    set_variable(name, TYPE_STRING, value);
    free(value);
}
