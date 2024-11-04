#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "../util/utility.h"
#include "../util/variables.h"
#include "../bytecode.h"
#include "types.h"

void handle_char(FILE *output, const char **ptr, const char *var_name, int var_index, VarStorageType storage_type) {
    char char_value = '\0';

    if (**ptr == '\'') {
        (*ptr)++;
        char_value = **ptr;
        (*ptr)++;
        if (**ptr != '\'') {
            handle_error("Invalid char value, expected a single character in quotes.");
        }
        (*ptr)++;
    } else {
        char referenced_var_name[256];
        int ref_len = 0;

        while (isalnum(**ptr)) {
            referenced_var_name[ref_len++] = *(*ptr)++;
        }
        referenced_var_name[ref_len] = '\0';

        int referenced_var_index = find_variable_index(referenced_var_name);
        if (referenced_var_index == -1 || variables[referenced_var_index].type != CHAR) {
            handle_error("Invalid character value, expected a char literal or another char variable.");
        }

        char_value = variables[referenced_var_index].value.char_val;
    }

    assign_variable_value(var_index, var_name, CHAR, storage_type, &char_value, sizeof(char));
    write_to_output(output, 0x0A, strlen(var_name), var_name, &char_value, sizeof(char));
}
