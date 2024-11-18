#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/bytecode.h"

void handle_char(const char *args, FILE *output) {
    fwrite(&(unsigned char) { CHAR_OP }, 1, 1, output);

    char name[32];
    char value;
    char *colon_ptr = strchr(args, ':');
    char *arrow_ptr = strstr(args, "->");

    if (!colon_ptr || !arrow_ptr) {
	fprintf(stderr, "Error: Invalid char assignment syntax\n");
	exit(ERR_MALFORMED_PRINT);
    }

    size_t name_len = colon_ptr - args;
    if (name_len >= sizeof(name)) {
	fprintf(stderr, "Error: Variable name is too long\n");
	exit(ERR_MALFORMED_PRINT);
    }
    strncpy(name, args, name_len);
    name[name_len] = '\0';

    char *value_start = strchr(arrow_ptr, '\'');
    if (!value_start)
	value_start = strchr(arrow_ptr, '"');
    if (!value_start || !value_start[1] || value_start[2] != value_start[0]) {
	fprintf(stderr, "Error: Invalid char value in assignment\n");
	exit(ERR_MALFORMED_PRINT);
    }
    value = value_start[1];

    fwrite(&name_len, sizeof(size_t), 1, output);
    fwrite(name, sizeof(char), name_len, output);
    fwrite(&value, sizeof(char), 1, output);

    set_variable(name, TYPE_CHAR, &value);
}

void handle_char_opcode(FILE *input) {
    size_t name_len;
    fread(&name_len, sizeof(size_t), 1, input);

    char name[32];
    fread(name, sizeof(char), name_len, input);
    name[name_len] = '\0';

    char value;
    fread(&value, sizeof(char), 1, input);

    set_variable(name, TYPE_CHAR, &value);
}
