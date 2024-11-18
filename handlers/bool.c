#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../headers/bytecode.h"

static char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str))
	str++;
    if (*str == 0)
	return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
	end--;
    end[1] = '\0';
    return str;
}

void handle_bool(const char *args, FILE *output) {
    fwrite(&(unsigned char) { BOOL_OP }, 1, 1, output);

    char name[32];
    int value;
    char *colon_ptr = strchr(args, ':');
    char *arrow_ptr = strstr(args, "->");

    if (!colon_ptr || !arrow_ptr) {
	fprintf(stderr, "Error: Invalid bool assignment syntax\n");
	exit(ERR_MALFORMED_PRINT);
    }

    size_t name_len = colon_ptr - args;
    if (name_len >= sizeof(name)) {
	fprintf(stderr, "Error: Variable name is too long\n");
	exit(ERR_MALFORMED_PRINT);
    }
    strncpy(name, args, name_len);
    name[name_len] = '\0';

    char *value_str = trim_whitespace(arrow_ptr + 2);

    if (strcmp(value_str, "true") == 0 || strcmp(value_str, "1") == 0) {
	value = 1;
    } else if (strcmp(value_str, "false") == 0 || strcmp(value_str, "0") == 0) {
	value = 0;
    } else {
	fprintf(stderr, "Error: Invalid boolean value\n");
	exit(ERR_MALFORMED_PRINT);
    }

    fwrite(&name_len, sizeof(size_t), 1, output);
    fwrite(name, sizeof(char), name_len, output);
    fwrite(&value, sizeof(int), 1, output);

    set_variable(name, TYPE_BOOL, &value);
}

void handle_bool_opcode(FILE *input) {
    size_t name_len;
    fread(&name_len, sizeof(size_t), 1, input);

    char name[32];
    fread(name, sizeof(char), name_len, input);
    name[name_len] = '\0';

    int value;
    fread(&value, sizeof(int), 1, input);

    set_variable(name, TYPE_BOOL, &value);
}
