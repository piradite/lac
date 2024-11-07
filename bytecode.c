#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bytecode.h"
#include "util/utility.h"
#include "util/variables.h"
#include "data_types/types.h"

int find_variable_index(const char *var_name) {
    for (int i = 0; i < var_count; i++)
	if (strcmp(var_name, variables[i].name) == 0)
	    return i;
    return -1;
}

void check_variable_redefinition(int var_index, VarType new_type, VarStorageType storage_type) {
    if (var_index == -1)
	return;

    VarStorageType existing_storage = variables[var_index].storage_type;
    VarType existing_type = variables[var_index].type;

    if (existing_type == ANY && new_type != NONE && new_type != ANY) {
	variables[var_index].type = new_type;
	return;
    }

    if (existing_storage == CONST) {
	handle_error("Cannot change 'const' variable. It is immutable.");
    } else if (existing_storage == VAR && new_type != NONE && existing_type != new_type) {
	handle_error("Type mismatch during reassignment.");
    } else if (existing_storage == LET) {
	if (storage_type == LET) {
	    handle_error("Cannot redeclare a 'let' variable within the same scope.");
	}
	if (new_type != NONE && existing_type != new_type) {
	    handle_error("Type mismatch during reassignment.");
	}
    }
}


void write_to_output(FILE *output, char byte_code, int len, const char *name, void *data, size_t data_size) {
    char buffer[512];
    int offset = 1 + sizeof(int) + len + (data ? data_size : 0);

    buffer[0] = byte_code;
    memcpy(buffer + 1, &len, sizeof(int));
    memcpy(buffer + 1 + sizeof(int), name, len);

    if (data && data_size > 0) {
	memcpy(buffer + 1 + sizeof(int) + len, data, data_size);
    }

    fwrite(buffer, 1, offset, output);
}

void compile_to_bytecode(const char *source_code, const char *bytecode_file) {
    FILE *output = fopen(bytecode_file, "wb");
    if (!output) {
	handle_error("Could not create bytecode file");
    }

    const char *ptr = source_code;
    int last_was_newline = 0;
    char last_char = '\0';
    int has_content = 0;

    while (*ptr) {

	if ((*ptr == ';' && (*(ptr + 1) == ';' || *(ptr + 1) == '*'))) {
	    ptr += (*(ptr + 1) == ';') ? strcspn(ptr, "\n") : 2 + strcspn(ptr + 2, "*;");
	    continue;
	}

	if (strncmp(ptr, "sleep", 5) == 0 && isspace(*(ptr + 5))) {
		ptr += 5;
		while (*ptr == ' ' || *ptr == '\t') ptr++; 

		char *non_const_ptr = (char *)ptr;

		double duration = strtod(non_const_ptr, &non_const_ptr);
		ptr = non_const_ptr;

		int seconds = (int)duration;
		int milliseconds = (int)((duration - seconds) * 1000);

		fwrite(&(char){0x0C}, 1, 1, output);
		fwrite(&seconds, sizeof(int), 1, output);
		fwrite(&milliseconds, sizeof(int), 1, output);

		while (*ptr && !isspace(*ptr)) ptr++;
		continue;
	}

	if (strncmp(ptr, "print", 5) == 0) {
	    ptr += 5;
	    while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	    process_print_statement(output, &ptr);
	    continue;
	} else if (isalpha(*ptr)) {
	    char var_name[256];
	    int len = 0;
	    VarStorageType storage_type = VAR;
	    VarType expected_type = NONE;

	    while (isalnum(*ptr))
		var_name[len++] = *ptr++;
	    var_name[len] = '\0';

	    int var_index = find_variable_index(var_name);

	    if (*ptr == ':')
		ptr++;
	    while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	    if (var_index != -1 && (*ptr == '"' || isdigit(*ptr) || (*ptr == '\''))) {
		expected_type = variables[var_index].type;
	    } else {
		if (var_index != -1) {
		    storage_type = variables[var_index].storage_type;
		    expected_type = variables[var_index].type;
		} else if (strncmp(ptr, "const", 5) == 0) {
		    storage_type = CONST;
		    ptr += 5;
		} else if (strncmp(ptr, "let", 3) == 0) {
		    storage_type = LET;
		    ptr += 3;
		} else if (strncmp(ptr, "var", 3) == 0) {
		    storage_type = VAR;
		    ptr += 3;
		} else if (strncmp(ptr, "any", 3) == 0) {
		    storage_type = VAR;
		    expected_type = ANY;
		    ptr += 3;
		} else if (var_index == -1) {
		    expected_type = variables[var_index].type;
		    check_variable_redefinition(var_index, expected_type, storage_type);
		} else {
		    handle_error("Variable not declared before use.");
		}
	    }

	    while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	    const char *types[] = { "int ->", "string ->", "float ->", "bool ->", "char ->", "any ->" };
	    int expected_types[] = { INT, STRING, FLOAT, BOOL, CHAR };
	    size_t num_types = sizeof(types) / sizeof(types[0]);

	    for (size_t i = 0; i < num_types; i++) {
		if (strncmp(ptr, types[i], strlen(types[i])) == 0) {
		    ptr += strlen(types[i]);
		    expected_type = expected_types[i];
		    break;
		}
	    }

	    check_variable_redefinition(var_index, expected_type, storage_type);

	    if (expected_type == NONE && var_index != -1) {
		expected_type = variables[var_index].type;
	    }

	    while (*ptr == ' ') ptr++;
		
	    if (expected_type == ANY) {
		if (*ptr == '-' && *(ptr + 1) == '>') {
		    ptr += 2;
		}
		if (*ptr == ' ' || *ptr == '\t') {
		    ptr++;
		}
		if (*ptr == '\n' || *ptr == '\0') {
		    return;
		}

		if (isdigit(*ptr) || (*ptr == '-' || *ptr == '+') && isdigit(*(ptr + 1))) {
		    if (strchr(ptr, '.')) {
			expected_type = FLOAT;
		    } else {
			expected_type = INT;
		    }
		} else if (*ptr == '"') {
		    expected_type = STRING;
		} else if (strncmp(ptr, "true", 4) == 0 || strncmp(ptr, "false", 5) == 0) {
		    expected_type = BOOL;
		} else if (*ptr == '\'' && *(ptr + 2) == '\'') {
		    expected_type = CHAR;
		} else {
		    fprintf(stderr, "Cannot infer type for: %s\n", ptr);
		    handle_error("Unsupported type for ANY");
		}
	    }

	    switch (expected_type) {
	    case INT:
		handle_int(output, &ptr, var_name, var_index, storage_type);
		break;
	    case STRING:
		handle_string(output, &ptr, var_name, var_index, storage_type);
		break;
	    case FLOAT:
		handle_float(output, &ptr, var_name, var_index, storage_type);
		break;
	    case BOOL:
		handle_bool(output, &ptr, var_name, var_index, storage_type);
		break;
	    case CHAR:
		handle_char(output, &ptr, var_name, var_index, storage_type);
		break;
	    default:
		handle_error("Unsupported type");
		break;
	    }

	}

	last_char = *ptr;
	ptr++;
    }

    if (last_char != '\n' ? (has_content = 0, 1) : has_content) {
	write_to_output(output, 0x02, 0, NULL, NULL, sizeof(char));
    }

    fclose(output);
}
