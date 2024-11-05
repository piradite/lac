#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "../util/utility.h"
#include "../util/variables.h"
#include "../bytecode.h"
#include "types.h"

void handle_string(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type) {
	int len = 0;
    if (**ptr == '"') {
	(*ptr)++;
	const char *start = *ptr;
	while (**ptr != '"' && **ptr != '\0')
	    (*ptr)++;
	size_t len_str = *ptr - start;
	(*ptr)++;

	assign_variable_value(var_index, var_name, STRING, storage_type, (void *)start, len_str);
	fwrite(&(char) { 0x05 }, sizeof(char), 1, output);
	fwrite(&len, sizeof(int), 1, output);
	fwrite(var_name, sizeof(char), len, output);
	fwrite(&len_str, sizeof(int), 1, output);
	fwrite(start, sizeof(char), len_str, output);
    }
}
