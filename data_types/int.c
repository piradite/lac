#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "../util/utility.h"
#include "../util/variables.h"
#include "../bytecode.h"
#include "types.h"

void handle_int(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type) {
    int int_value = 0;
    char referenced_var_name[256];
    int ref_len = 0;
    bool is_literal = isdigit(**ptr) || **ptr == '-';

    if (is_literal) {
	int_value = strtol(*ptr, (char **)ptr, 10);
	while (isdigit(**ptr) || **ptr == '-') {
	    (*ptr)++;
	}
    } else {
	while (isalnum(**ptr)) {
	    referenced_var_name[ref_len++] = **ptr;
	    (*ptr)++;
	}
	referenced_var_name[ref_len] = '\0';

	int referenced_var_index = find_variable_index(referenced_var_name);
	if (referenced_var_index == -1 || variables[referenced_var_index].type != INT) {
	    handle_error("Invalid int value, expected an int literal or another int variable.");
	}
	int_value = variables[referenced_var_index].value.int_val;
    }

    assign_variable_value(var_index, var_name, INT, storage_type, &int_value, sizeof(int));
    write_to_output(output, 0x04, strlen(var_name), var_name, &int_value, sizeof(int));
}
