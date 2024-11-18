#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/bytecode.h"

void handle_float(const char *args, FILE *output) {
    fwrite(&(unsigned char) { FLOAT_OP }, 1, 1, output);

    char name[32];
    float value;
    char *colon_ptr = strchr(args, ':');
    char *arrow_ptr = strstr(args, "->");

    if (!colon_ptr || !arrow_ptr) {
	fprintf(stderr, "Error: Invalid float assignment syntax\n");
	exit(ERR_MALFORMED_PRINT);
    }

    size_t name_len = colon_ptr - args;
    if (name_len >= sizeof(name)) {
	fprintf(stderr, "Error: Variable name is too long\n");
	exit(ERR_MALFORMED_PRINT);
    }
    strncpy(name, args, name_len);
    name[name_len] = '\0';

    if (sscanf(arrow_ptr + 2, "%f", &value) != 1) {
	fprintf(stderr, "Error: Invalid float value in assignment\n");
	exit(ERR_MALFORMED_PRINT);
    }

    fwrite(&name_len, sizeof(size_t), 1, output);
    fwrite(name, sizeof(char), name_len, output);
    fwrite(&value, sizeof(float), 1, output);
    set_variable(name, TYPE_FLOAT, &value);
}

void handle_float_opcode(FILE *input) {
    size_t name_len;
    fread(&name_len, sizeof(size_t), 1, input);

    char name[32];
    fread(name, sizeof(char), name_len, input);
    name[name_len] = '\0';

    float value;
    fread(&value, sizeof(float), 1, input);

    set_variable(name, TYPE_FLOAT, &value);
}
