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
    if (**ptr == '"') {
	(*ptr)++;
	const char *start = *ptr;
	while (**ptr != '"' && **ptr != '\0')
	    (*ptr)++;
	size_t len_str = *ptr - start;
	(*ptr)++;

	assign_variable_value(var_index, var_name, STRING, storage_type, (void *)start, len_str);
	write_to_output(output, 0x05, strlen(var_name), var_name, &len_str, sizeof(int));
	write_to_output(output, 0x05, len_str, var_name, (void *)start, len_str);
    }
}
