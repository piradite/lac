#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "../util/utility.h"
#include "../util/variables.h"
#include "../bytecode.h"
#include "types.h"

void handle_bool(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type) {
    int bool_value = 0;

    if (strncmp(*ptr, "true", 4) == 0) {
        *ptr += 4;
        bool_value = 1;
    } else if (strncmp(*ptr, "false", 5) == 0) {
        *ptr += 5;
        bool_value = 0;
    } else {
        char referenced_var_name[256];
        int ref_len = 0;

        while (isalnum(**ptr)) {
            referenced_var_name[ref_len++] = *(*ptr)++;
        }
        referenced_var_name[ref_len] = '\0';

        int referenced_var_index = find_variable_index(referenced_var_name);
        if (referenced_var_index == -1 || variables[referenced_var_index].type != BOOL) {
            handle_error("Invalid boolean value, expected 'true', 'false', or another boolean variable.");
        }

        bool_value = variables[referenced_var_index].value.bool_val;
    }

    assign_variable_value(var_index, var_name, BOOL, storage_type, &bool_value, sizeof(int));
    write_to_output(output, 0x09, strlen(var_name), var_name, &bool_value, sizeof(int));
}
