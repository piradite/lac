#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "../util/utility.h"
#include "../util/variables.h"
#include "../bytecode.h"
#include "types.h"

void handle_float(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type) {
    float float_value = 0.0;

    if (isdigit(**ptr) || **ptr == '-') {
        float_value = strtof(*ptr, (char **)ptr);
    } else {
        char referenced_var_name[256];
        int ref_len = 0;
        while (isalnum(**ptr)) {
            referenced_var_name[ref_len++] = *(*ptr)++;
        }
        referenced_var_name[ref_len] = '\0';

        int referenced_var_index = find_variable_index(referenced_var_name);
        if (referenced_var_index == -1 || variables[referenced_var_index].type != FLOAT) {
            handle_error("Invalid float value, expected a float literal or another float variable.");
        }
        float_value = variables[referenced_var_index].value.float_val;
    }

    assign_variable_value(var_index, var_name, FLOAT, storage_type, &float_value, sizeof(float));
    write_to_output(output, 0x08, strlen(var_name), var_name, &float_value, sizeof(float));
}
