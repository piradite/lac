#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utility.h"
#include "variables.h"
#include "../bytecode.h"

void handle_error(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(EXIT_FAILURE);
}

char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
	handle_error("Could not open file");
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *content = malloc(length + 1);
    if (!content) {
	handle_error("Memory allocation failed");
    }
    fread(content, 1, length, file);
    fclose(file);
    content[length] = '\0';
    return content;
}

void process_print_statement(FILE *output, const char **ptr) {
    int first_item = 1;
    int output_buffer[256];
    int buffer_index = 0;

    while (**ptr && **ptr != '\n') {
	if (**ptr == ';' && *(*ptr + 1) == ';') {
	    break;
	}

	if (**ptr == '"') {
	    if (!first_item) {
		output_buffer[buffer_index++] = ' ';
	    }
	    first_item = 0;

	    fwrite(&(char) { 0x01 }, 1, 1, output);
	    const char *start = ++(*ptr);
	    while (**ptr != '"' && **ptr) {
		(*ptr)++;
	    }
	    size_t len = *ptr - start;
	    fwrite(&len, sizeof(size_t), 1, output);
	    fwrite(start, 1, len, output);
	    (*ptr)++;
	}

	else if (isalnum(**ptr)) {
	    if (!first_item) {
		output_buffer[buffer_index++] = ' ';
	    }
	    first_item = 0;

	    char var_name[256];
	    int len = 0;
	    while (isalnum(**ptr)) {
		var_name[len++] = *(*ptr)++;
	    }
	    var_name[len] = '\0';

	    int var_index = find_variable_index(var_name);
	    if (var_index == -1) {
		handle_error("Variable not declared");
	    }

	    if (variables[var_index].type == STRING) {
		fwrite(&(char) { 0x01 }, 1, 1, output);
		size_t str_len = strlen(variables[var_index].value.string_val);
		fwrite(&str_len, sizeof(size_t), 1, output);
		fwrite(variables[var_index].value.string_val, 1, str_len, output);
	    } else {
		fwrite(&(char) { 0x03 }, 1, 1, output);
		fwrite(&len, sizeof(int), 1, output);
		fwrite(var_name, 1, len, output);
	    }
	}

	if (**ptr == ',') {
	    if (buffer_index > 0 || **ptr == ',') {
		fwrite(&(char) { 0x06 }, 1, 1, output);

	    }
	    (*ptr)++;
	    while (**ptr == ' ' || **ptr == '\t') {
		(*ptr)++;
	    }
	    first_item = 1;
	} else if (**ptr == '.') {
	    fwrite(&(char) { 0x02 }, 1, 1, output);
	    (*ptr)++;
	    while (**ptr == ' ' || **ptr == '\t') {
		(*ptr)++;
	    }
	    first_item = 1;
	} else if (**ptr == '&') {
		(*ptr)++;
		char var_name[256];
		int len = 0;
		while (isalnum(**ptr)) {
			var_name[len++] = *(*ptr)++;
		}
		var_name[len] = '\0';

		int var_index = find_variable_index(var_name);
		if (var_index == -1) {
			handle_error("Variable not declared");
		}

		fwrite(&(char) { 0x0B }, 1, 1, output);
		fwrite(&len, sizeof(int), 1, output);
		fwrite(var_name, 1, len, output);
	}


	while (**ptr == ' ' || **ptr == '\t') {
	    (*ptr)++;
	}
    }

    if (**ptr == '\n') {
	fwrite(&(char) { 0x02 }, 1, 1, output);
    }
}
