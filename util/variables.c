#include "variables.h"
#include "utility.h"
#include <string.h>
#include <ctype.h>
Variable variables[256];
int var_count = 0;
void assign_variable_value(int var_index, const char *var_name, VarType expected_type, VarStorageType storage_type, void *value, size_t value_size) {
    if (var_index == -1) {
	strcpy(variables[var_count].name, var_name);
	variables[var_count].storage_type = storage_type;

	if (expected_type == ANY) {
	    if (value_size == sizeof(int) && isdigit(*(char *)value)) {
		expected_type = INT;
		variables[var_count].value.int_val = *(int *)value;
	    } else if (value_size == sizeof(float)) {
		expected_type = FLOAT;
		variables[var_count].value.float_val = *(float *)value;
	    } else if (value_size == sizeof(char) && strlen((char *)value) == 1) {
		expected_type = CHAR;
		variables[var_count].value.char_val = *(char *)value;
	    } else if (strlen((char *)value) < sizeof(variables[var_count].value.string_val)) {
		expected_type = STRING;
		strncpy(variables[var_count].value.string_val, (char *)value, value_size);
		variables[var_count].value.string_val[value_size] = '\0';
	    } else {
		handle_error("Unsupported type for ANY");
	    }
	} else {
	    variables[var_count].type = expected_type;
	    (expected_type == INT) ? (variables[var_count].value.int_val = *(int *)value) :
		(expected_type == FLOAT) ? (variables[var_count].value.float_val = *(float *)value) :
		(expected_type == BOOL) ? (variables[var_count].value.bool_val = *(int *)value) :
		(expected_type == CHAR) ? (variables[var_count].value.char_val = *(char *)value) :
		(expected_type == STRING) ? (strncpy(variables[var_count].value.string_val, (char *)value, value_size), variables[var_count].value.string_val[value_size] = '\0') :
		handle_error("Unsupported type");
	}

	variables[var_count].type = expected_type;
	var_count++;
    } else {
	if (variables[var_index].type == ANY) {
	    variables[var_index].type = expected_type;
	}

	if (variables[var_index].type != expected_type) {
	    handle_error("Type mismatch in redeclaration");
	    return;
	}
	if (expected_type == STRING)
	    memset(variables[var_index].value.string_val, 0, sizeof(variables[var_index].value.string_val));

	(expected_type == INT) ? (variables[var_index].value.int_val = *(int *)value) :
	    (expected_type == FLOAT) ? (variables[var_index].value.float_val = *(float *)value) :
	    (expected_type == BOOL) ? (variables[var_index].value.bool_val = *(int *)value) :
	    (expected_type == CHAR) ? (variables[var_index].value.char_val = *(char *)value) :
	    (expected_type == STRING) ? (strncpy(variables[var_index].value.string_val, (char *)value, value_size), variables[var_index].value.string_val[value_size] = '\0') :
	    handle_error("Unsupported type");
    }
}
